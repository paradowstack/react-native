/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace facebook::react {

class RuntimeSchedulerResizeObserverDelegate {
 public:
  virtual ~RuntimeSchedulerResizeObserverDelegate() = default;

  virtual void runResizeObservations() = 0;
};

} // namespace facebook::react
