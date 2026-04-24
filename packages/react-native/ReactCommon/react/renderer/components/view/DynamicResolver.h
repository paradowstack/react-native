/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/DynamicPropertiesMap.h>

namespace facebook::yoga {
class StyleLength;
class StyleSizeLength;
} // namespace facebook::yoga

namespace facebook::react {

struct DynamicResolver {
  const DynamicPropertiesMap& propertiesMap;
  const DynamicResolveContext& context;

  Float resolve(const FloatDynamic& value) const;
  Float resolve(const ValueUnit& value, float percentRef = 0.0f) const;
  Float resolve(
      const facebook::yoga::StyleLength& length,
      float percentRef = 0.0f) const;
  Float resolve(
      const facebook::yoga::StyleSizeLength& length,
      float percentRef = 0.0f) const;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic(const FloatDynamic& value) const;
  folly::dynamic toDynamic(const ValueUnit& value, float percentRef = 0.0f)
      const;
  folly::dynamic toDynamic(
      const facebook::yoga::StyleLength& length,
      float percentRef = 0.0f) const;
  folly::dynamic toDynamic(
      const facebook::yoga::StyleSizeLength& length,
      float percentRef = 0.0f) const;
#endif

 private:
  Float resolve(DynamicPropertyId id, float referenceLength = 0.0f) const;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic(DynamicPropertyId id, float referenceLength = 0.0f)
      const;
#endif
};

} // namespace facebook::react
