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
  // `display: 'none'` sets the Hidden trait when props commit, possibly before
  // layout updates `displayType`.
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
    ResizeObserverBoxOptions boxOptions,
    uint64_t observationSequence)
    : resizeObserverId_{resizeObserverId},
      targetShadowNodeFamily_{std::move(targetShadowNodeFamily)},
      boxOptions_{boxOptions},
      observationSequence_{observationSequence} {}

// Partially equivalent to
// https://w3c.github.io/csswg-drafts/resize-observer/#broadcast-active-resize-observations
std::optional<ResizeObserverEntry> ResizeObserver::updateStateIfNeeded(
    const RootShadowNode& rootShadowNode) {
  auto ancestors = targetShadowNodeFamily_->getAncestors(rootShadowNode);
  const auto* targetShadowNode =
      ancestors.empty() ? nullptr : getTargetShadowNode(ancestors);

  if (targetShadowNode == nullptr) {
    // Target left the tree. Per spec, removal fires one final 0x0 entry. Keep
    // `lastReportedSize_` at 0x0 so we don't re-deliver, and mark detached to
    // stop re-checking until it's reinserted (via the dirty-family path).
    auto zeroSize = Size{0, 0};
    auto observedSize =
        getObservedSize(boxOptions_, zeroSize, zeroSize, zeroSize);

    const auto alreadyDelivered = lastReportedSize_.has_value() &&
        lastReportedSize_.value() == observedSize;
    detached_ = true;
    if (alreadyDelivered) {
      return std::nullopt;
    }

    lastReportedSize_ = observedSize;

    return makeResizeObserverEntry(
        resizeObserverId_,
        targetShadowNodeFamily_,
        zeroSize,
        zeroSize,
        zeroSize,
        Rect{.origin = {0, 0}, .size = zeroSize});
  }

  // Attached: clear any detached state.
  detached_ = false;

  const auto isInitialDelivery = !lastReportedSize_.has_value();

  // Per spec, `display: none` reports zero-sized boxes. Use the Hidden trait
  // instead of possibly-stale layout metrics.
  if (isTargetHidden(*targetShadowNode)) {
    auto zeroSize = Size{0, 0};
    auto zeroContentRect = Rect{.origin = {0, 0}, .size = zeroSize};
    auto observedSize =
        getObservedSize(boxOptions_, zeroSize, zeroSize, zeroSize);

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
      ancestors, {.includeTransform = false, .includeViewportOffset = true});

  // No relative layout (e.g. a display:none ancestor). Emit nothing (no
  // reliable box) and reset `lastReportedSize_` so the next successful read
  // delivers fresh.
  if (layoutMetrics == EmptyLayoutMetrics) {
    lastReportedSize_.reset();
    return std::nullopt;
  }

  auto borderBoxSize = layoutMetrics.frame.size;

  // RN's `contentInsets` is border + padding per side, matching the Web
  // content-box. Clamp to zero: a frame can be smaller than its insets, but a
  // content box is never negative.
  auto contentFrame = layoutMetrics.getContentFrame();
  auto contentBoxSize = Size{
      .width = std::max(Float{0}, contentFrame.size.width),
      .height = std::max(Float{0}, contentFrame.size.height)};

  // Per spec, `contentRect`'s origin is the offset of the content box from
  // the padding box (i.e. the paddings only, excluding borders).
  // https://w3c.github.io/csswg-drafts/resize-observer/#dom-resizeobserverentry-contentrect
  auto contentRect = Rect{
      .origin =
          {.x = layoutMetrics.contentInsets.left -
               layoutMetrics.borderWidth.left,
           .y =
               layoutMetrics.contentInsets.top - layoutMetrics.borderWidth.top},
      .size = contentBoxSize};

  // Per spec the device-pixel-content-box holds integers. Round each axis
  // (best-effort: we have no pixel-snapped origin or sibling to align to).
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
