/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/DynamicPropertiesMap.h>

namespace facebook::react {

struct DynamicResolver {
  const DynamicPropertiesMap& propertiesMap;
  const DynamicResolveContext& context;

  FloatDynamic resolve(const FloatDynamic& value) const {
    if (value.isDynamic() && !propertiesMap.empty()) {
      auto it = propertiesMap.find(value.asDynamicId());
      if (it != propertiesMap.end()) {
        return FloatDynamic{it->second.resolve(
            0.0f, context.viewportWidth(), context.viewportHeight())};
      }
    }
    return value;
  }

  ValueUnit resolve(const ValueUnit& value, float percentRef) const {
    if (value.isDynamic() && !propertiesMap.empty()) {
      auto it = propertiesMap.find(value.asDynamicId());
      if (it != propertiesMap.end()) {
        return ValueUnit{
            it->second.resolve(
                percentRef, context.viewportWidth(), context.viewportHeight()),
            UnitType::Point};
      }
    }
    return value;
  }
};

} // namespace facebook::react
