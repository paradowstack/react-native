/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ResizeObserver.h"
#include <utility>

namespace facebook::react {

ResizeObserver::ResizeObserver(
    ResizeObserverObserverId resizeObserverId,
    ShadowNodeFamily::Shared targetShadowNodeFamily,
    ResizeObserverBoxOptions boxOptions)
    : resizeObserverId_(resizeObserverId),
      targetShadowNodeFamily_(std::move(targetShadowNodeFamily)),
      boxOptions_(boxOptions) {}

std::optional<ResizeObserverEntry> ResizeObserver::updateStateIfNeeded(
    const RootShadowNode& /*rootShadowNode*/) {
  // TODO(ResizeObserver): implement
  return std::nullopt;
}

} // namespace facebook::react
