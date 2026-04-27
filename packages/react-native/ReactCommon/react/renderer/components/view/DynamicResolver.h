/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/graphics/NumericValue.h>

namespace facebook::yoga {
class StyleLength;
class StyleSizeLength;
} // namespace facebook::yoga

namespace facebook::react {

struct DynamicResolver {
  const DynamicPropertiesMap& propertiesMap;
  const DynamicResolveContext& context;

  Float resolveNumber(const UntypedNumericValue& value) const;
  Float resolveLength(const UntypedNumericValue& value) const;
  Float resolveLengthOrPercentage(
      const UntypedNumericValue& value,
      float percentRef = 0.0f) const;
  Float resolve(
      const facebook::yoga::StyleLength& length,
      float percentRef = 0.0f) const;
  Float resolve(
      const facebook::yoga::StyleSizeLength& length,
      float percentRef = 0.0f) const;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamicNumber(const UntypedNumericValue& value) const;
  folly::dynamic toDynamicLength(const UntypedNumericValue& value) const;
  folly::dynamic toDynamicLengthOrPercentage(
      const UntypedNumericValue& value,
      float percentRef = 0.0f) const;
  folly::dynamic toDynamic(
      const facebook::yoga::StyleLength& length,
      float percentRef = 0.0f) const;
  folly::dynamic toDynamic(
      const facebook::yoga::StyleSizeLength& length,
      float percentRef = 0.0f) const;
#endif

 private:
  Float resolve(
      const UntypedNumericValue& value,
      NumericValueDomain domain,
      float referenceLength = 0.0f) const;
  Float resolve(DynamicPropertyId id, float referenceLength = 0.0f) const;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic(
      const UntypedNumericValue& value,
      NumericValueDomain domain,
      float referenceLength = 0.0f) const;
  folly::dynamic toDynamic(DynamicPropertyId id, float referenceLength = 0.0f)
      const;
#endif
};

} // namespace facebook::react
