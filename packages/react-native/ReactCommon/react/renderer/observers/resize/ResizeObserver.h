/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/root/RootShadowNode.h>
#include <react/renderer/core/ShadowNodeFamily.h>
#include <react/renderer/graphics/Rect.h>
#include <react/renderer/graphics/Size.h>
#include <memory>
#include <optional>

namespace facebook::react {

using ResizeObserverObserverId = int32_t;

// Corresponds to the `box` option of `ResizeObserver#observe`.
// https://w3c.github.io/csswg-drafts/resize-observer/#resize-observer-box-options
enum class ResizeObserverBoxOptions { ContentBox, BorderBox };

struct ResizeObserverEntry {
  ResizeObserverObserverId resizeObserverId;
  ShadowNodeFamily::Shared shadowNodeFamily;
  Rect contentRect;
  Size borderBoxSize;
  Size contentBoxSize;

  bool sameShadowNodeFamily(
      const ShadowNodeFamily& otherShadowNodeFamily) const {
    return std::addressof(*shadowNodeFamily) ==
        std::addressof(otherShadowNodeFamily);
  }
};

class ResizeObserver {
 public:
  ResizeObserver(
      ResizeObserverObserverId resizeObserverId,
      ShadowNodeFamily::Shared targetShadowNodeFamily,
      ResizeObserverBoxOptions boxOptions);

  // Partially equivalent to
  // https://w3c.github.io/csswg-drafts/resize-observer/#broadcast-active-resize-observations
  std::optional<ResizeObserverEntry> updateStateIfNeeded(
      const RootShadowNode& rootShadowNode);

  ResizeObserverObserverId getResizeObserverId() const {
    return resizeObserverId_;
  }

  ShadowNodeFamily::Shared getTargetShadowNodeFamily() const {
    return targetShadowNodeFamily_;
  }

  ResizeObserverBoxOptions getBoxOptions() const {
    return boxOptions_;
  }

 private:
  ResizeObserverObserverId resizeObserverId_;
  ShadowNodeFamily::Shared targetShadowNodeFamily_;
  ResizeObserverBoxOptions boxOptions_;

  // Tracks the "last reported size" for this observation, per the
  // ResizeObserver spec.
  // https://w3c.github.io/csswg-drafts/resize-observer/#resize-observer-last-reported-size-slot
  std::optional<Size> lastReportedSize_;
};

} // namespace facebook::react
