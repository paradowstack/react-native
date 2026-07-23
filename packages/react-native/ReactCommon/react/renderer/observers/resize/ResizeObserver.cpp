/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ResizeObserver.h"
#include <react/renderer/core/LayoutableShadowNode.h>
#include <react/renderer/core/ShadowNode.h>
#include <react/renderer/core/ShadowNodeTraits.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace facebook::react {

namespace {

const ShadowNode* getTargetShadowNode(
    const ShadowNodeFamily::AncestorList& ancestors) {
  if (ancestors.empty()) {
    return nullptr;
  }

  const auto& parentChildPair = ancestors.back();
  return parentChildPair.first.get()
      .getChildren()
      .at(parentChildPair.second)
      .get();
}

bool isTargetHidden(const ShadowNode& targetShadowNode) {
  // `display: 'none'` sets the Hidden trait on View nodes when props are
  // committed, which can be ahead of the last laid-out `displayType` / frame.
  if (targetShadowNode.getTraits().check(ShadowNodeTraits::Trait::Hidden)) {
    return true;
  }

  if (const auto* layoutableShadowNode =
          dynamic_cast<const LayoutableShadowNode*>(&targetShadowNode)) {
    return layoutableShadowNode->getLayoutMetrics().displayType ==
        DisplayType::None;
  }

  return false;
}

Size getObservedSize(
    ResizeObserverBoxOptions boxOptions,
    Size borderBoxSize,
    Size contentBoxSize,
    Size devicePixelContentBoxSize) {
  if (boxOptions == ResizeObserverBoxOptions::ContentBox) {
    return contentBoxSize;
  }
  if (boxOptions == ResizeObserverBoxOptions::DevicePixelContentBox) {
    return devicePixelContentBoxSize;
  }
  return borderBoxSize;
}

ResizeObserverEntry makeResizeObserverEntry(
    ResizeObserverObserverId resizeObserverId,
    const ShadowNodeFamily::Shared& targetShadowNodeFamily,
    Size borderBoxSize,
    Size contentBoxSize,
    Size devicePixelContentBoxSize,
    Rect contentRect) {
  return ResizeObserverEntry{
      resizeObserverId,
      targetShadowNodeFamily,
      contentRect,
      borderBoxSize,
      contentBoxSize,
      devicePixelContentBoxSize};
}

} // namespace

ResizeObserver::ResizeObserver(
    ResizeObserverObserverId resizeObserverId,
    ShadowNodeFamily::Shared targetShadowNodeFamily,
    ResizeObserverBoxOptions boxOptions)
    : resizeObserverId_(resizeObserverId),
      targetShadowNodeFamily_(std::move(targetShadowNodeFamily)),
      boxOptions_(boxOptions) {}

// Partially equivalent to
// https://w3c.github.io/csswg-drafts/resize-observer/#broadcast-active-resize-observations
std::optional<ResizeObserverEntry> ResizeObserver::updateStateIfNeeded(
    const RootShadowNode& rootShadowNode) {
  auto ancestors = targetShadowNodeFamily_->getAncestors(rootShadowNode);

  if (ancestors.empty()) {
    // The target is not part of the tree. If it is later reinserted, treat it
    // as a brand new observation.
    lastReportedSize_.reset();
    return std::nullopt;
  }

  const auto* targetShadowNode = getTargetShadowNode(ancestors);
  if (targetShadowNode == nullptr) {
    lastReportedSize_.reset();
    return std::nullopt;
  }

  const bool isInitialDelivery = !lastReportedSize_.has_value();

  // Per spec (and browser behavior), `display: none` triggers a notification
  // with zero-sized boxes. Use the Hidden trait so we do not rely on possibly
  // stale layout metrics from before the hide commit was laid out.
  if (isTargetHidden(*targetShadowNode)) {
    Size zeroSize{0, 0};
    Rect zeroContentRect{.origin = {0, 0}, .size = zeroSize};
    auto observedSize = getObservedSize(
        boxOptions_, zeroSize, zeroSize, zeroSize);

    if (!isInitialDelivery && lastReportedSize_.value() == observedSize) {
      return std::nullopt;
    }

    lastReportedSize_ = observedSize;

    return makeResizeObserverEntry(
        resizeObserverId_,
        targetShadowNodeFamily_,
        zeroSize,
        zeroSize,
        zeroSize,
        zeroContentRect);
  }

  auto layoutMetrics = LayoutableShadowNode::computeRelativeLayoutMetrics(
      ancestors,
      {.includeTransform = false, .includeViewportOffset = true});

  if (layoutMetrics == EmptyLayoutMetrics) {
    lastReportedSize_.reset();
    return std::nullopt;
  }

  auto borderBoxSize = layoutMetrics.frame.size;

  // In RN's `LayoutMetrics`, `contentInsets` represents the combined width of
  // border and padding in each direction, which matches the Web's definition
  // of the content-box. The size is clamped to zero because a frame can be
  // smaller than its insets (e.g. `width` smaller than the sum of the
  // horizontal borders), but a content box can never have a negative size.
  auto contentFrame = layoutMetrics.getContentFrame();
  Size contentBoxSize{
      .width = std::max(Float{0}, contentFrame.size.width),
      .height = std::max(Float{0}, contentFrame.size.height)};

  // Per spec, `contentRect`'s origin is the offset of the content box from
  // the padding box (i.e. the paddings only, excluding borders).
  // https://w3c.github.io/csswg-drafts/resize-observer/#dom-resizeobserverentry-contentrect
  Rect contentRect{
      .origin =
          {.x = layoutMetrics.contentInsets.left -
               layoutMetrics.borderWidth.left,
           .y = layoutMetrics.contentInsets.top -
               layoutMetrics.borderWidth.top},
      .size = contentBoxSize};

  // Per spec, the device-pixel-content-box must contain integer values.
  // Round each axis independently: unlike Yoga's edge-snapping, we don't have
  // a pixel-snapped content-box origin to tile against (insets come out of
  // Yoga unsnapped and the origin is an accumulated sum of rounded relative
  // offsets), and a single observed element has no sibling to align with, so
  // rounding the dimension is the correct best-effort here.
  auto devicePixelContentBoxSize = Size{
      std::round(contentBoxSize.width * layoutMetrics.pointScaleFactor),
      std::round(contentBoxSize.height * layoutMetrics.pointScaleFactor)};

  auto observedSize = getObservedSize(
      boxOptions_, borderBoxSize, contentBoxSize, devicePixelContentBoxSize);

  // Skip when the observed box is unchanged. The first delivery always runs
  // (including 0x0 content-box), matching browser behavior on `observe()`.
  if (!isInitialDelivery && lastReportedSize_.value() == observedSize) {
    return std::nullopt;
  }

  lastReportedSize_ = observedSize;

  return makeResizeObserverEntry(
      resizeObserverId_,
      targetShadowNodeFamily_,
      borderBoxSize,
      contentBoxSize,
      devicePixelContentBoxSize,
      contentRect);
}

} // namespace facebook::react
