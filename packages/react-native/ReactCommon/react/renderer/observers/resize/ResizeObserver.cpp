/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ResizeObserver.h"
#include <react/renderer/core/LayoutableShadowNode.h>
#include <algorithm>
#include <utility>

namespace facebook::react {

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

  auto layoutMetrics = ancestors.empty()
      ? EmptyLayoutMetrics
      : LayoutableShadowNode::computeRelativeLayoutMetrics(
            ancestors,
            {.includeTransform = false, .includeViewportOffset = true});

  if (layoutMetrics == EmptyLayoutMetrics) {
    // The target is not (or no longer) part of the tree rooted at
    // `rootShadowNode`. Per spec, if the element is later reinserted into the
    // tree it must be treated as a brand new observation, so we reset the
    // last reported size.
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

  auto devicePixelContentBoxSize = Size{
      contentBoxSize.width * layoutMetrics.pointScaleFactor,
      contentBoxSize.height * layoutMetrics.pointScaleFactor};

  auto observedSize = boxOptions_ == ResizeObserverBoxOptions::ContentBox
      ? contentBoxSize
      : boxOptions_ == ResizeObserverBoxOptions::DevicePixelContentBox
      ? devicePixelContentBoxSize
      : borderBoxSize;

  if (lastReportedSize_.has_value() &&
      lastReportedSize_.value() == observedSize) {
    return std::nullopt;
  }

  lastReportedSize_ = observedSize;

  return ResizeObserverEntry{
      resizeObserverId_,
      targetShadowNodeFamily_,
      contentRect,
      borderBoxSize,
      contentBoxSize,
      devicePixelContentBoxSize};
}

} // namespace facebook::react
