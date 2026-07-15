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
enum class ResizeObserverBoxOptions {
  ContentBox,
  BorderBox,
  DevicePixelContentBox
};

struct ResizeObserverEntry {
  ResizeObserverObserverId resizeObserverId;
  ShadowNodeFamily::Shared shadowNodeFamily;
  Rect contentRect;
  Size borderBoxSize;
  Size contentBoxSize;
  Size devicePixelContentBoxSize;

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

  // Whether this observation still needs its first delivery at the next
  // `runResizeObservations` step (just registered, or target was removed).
  bool needsInitialDeliveryCheck() const {
    return !lastReportedSize_.has_value();
  }

 private:
  ResizeObserverObserverId resizeObserverId_;
  ShadowNodeFamily::Shared targetShadowNodeFamily_;
  ResizeObserverBoxOptions boxOptions_;

  // Last delivered observed-box size. Empty until the first entry is produced.
  // Reset when the target leaves the tree so a reinsert is a new observation.
  std::optional<Size> lastReportedSize_;
};

} // namespace facebook::react
