/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <unordered_set>

#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/components/view/DynamicResolver.h>

namespace facebook::react {

class DynamicPropertiesHolder {
 public:
  virtual ~DynamicPropertiesHolder() = default;

  virtual void resolveProperties(const DynamicResolver& resolver) = 0;
  virtual void collectLiveResolvableIds(
      std::unordered_set<DynamicPropertyId>& ids) const = 0;

#ifdef RN_SERIALIZABLE_STATE
  virtual folly::dynamic getResolvedProps(
      const DynamicResolver& resolver) const = 0;
#endif
};

} // namespace facebook::react