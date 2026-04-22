/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <yoga/style/StyleLength.h>
#include <yoga/style/StyleSizeLength.h>

namespace facebook::react {

struct DynamicResolver {
  const DynamicPropertiesMap& propertiesMap;
  const DynamicResolveContext& context;

  Float resolve(const FloatDynamic& value) const {
    if (value.isDynamic() && !propertiesMap.empty()) {
      return FloatDynamic{resolve(value.asDynamicId())};
    }
    return value.value;
  }

  Float resolve(const ValueUnit& value, float percentRef = 0.0f) const {
    if (value.isDynamic()) {
      return ValueUnit{
          (float)resolve(value.asDynamicId(), percentRef), UnitType::Point};
    }
    return value;
  }

  Float resolve(
      const facebook::yoga::StyleLength& length,
      float percentRef = 0.0f) const {
    if (length.isDynamic()) {
      return resolve(length.callbackId(), percentRef);
    }
    return length.value().unwrap();
  }

  Float resolve(
      const facebook::yoga::StyleSizeLength& length,
      float percentRef = 0.0f) const {
    if (length.isDynamic()) {
      return resolve(length.callbackId(), percentRef);
    }
    return length.value().unwrap();
  }

 private:
  Float resolve(DynamicPropertyId id, float referenceLength = 0.0f) const {
    auto it = propertiesMap.find(id);
    if (it != propertiesMap.end()) {
      return it->second.resolve(
          referenceLength, context.viewportWidth(), context.viewportHeight());
    }
    return {};
  }
};

} // namespace facebook::react
