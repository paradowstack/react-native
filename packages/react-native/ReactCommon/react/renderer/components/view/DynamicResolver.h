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

  void resolve(const DynamicPropertyId& id, Float& result, float ref = 0.0f)
      const;
  Float resolveNumber(DynamicPropertyId id, float ref = 0.0f) const;
  Float resolveNumber(const UntypedNumericValue& value) const;
  Float resolveLength(const UntypedNumericValue& value) const;
  Float resolveLengthOrPercentage(
      const UntypedNumericValue& value,
      float percentRef = 0.0f) const;
  // Resolves a length-or-percentage dynamic value, preserving the concrete
  // length/percentage kind for non-dynamic values.
  LengthPercentageValue resolveLengthPercentage(
      const LengthPercentageValue& value,
      float percentRef = 0.0f) const;
  UntypedNumericValue resolveAny(
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
  folly::dynamic toDynamicAny(
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
  Float resolveCalcEntry(
      const TypedCalcEntry& entry,
      float referenceLength = 0.0f) const;
  Float resolveById(DynamicPropertyId id, float referenceLength = 0.0f) const;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamicCalcEntry(
      const TypedCalcEntry& entry,
      float referenceLength = 0.0f) const;
  folly::dynamic toDynamicById(
      DynamicPropertyId id,
      float referenceLength = 0.0f) const;
#endif
};

} // namespace facebook::react
