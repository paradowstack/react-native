/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "DynamicResolver.h"

#include <react/renderer/components/view/NumericValueConversions.h>

#include <yoga/style/StyleLength.h>
#include <yoga/style/StyleSizeLength.h>

#include <limits>

namespace facebook::react {

Float DynamicResolver::resolveNumber(const UntypedNumericValue& value) const {
  return resolve(value, NumericValueDomain::Number);
}

Float DynamicResolver::resolveLength(const UntypedNumericValue& value) const {
  return resolve(value, NumericValueDomain::Length);
}

Float DynamicResolver::resolveLengthOrPercentage(
    const UntypedNumericValue& value,
    float percentRef) const {
  return resolve(value, NumericValueDomain::LengthOrPercentage, percentRef);
}

Float DynamicResolver::resolve(
    const facebook::yoga::StyleLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return resolve(length.callbackId(), percentRef);
  }

  return length.resolve(percentRef, nullptr).unwrap();
}

Float DynamicResolver::resolve(
    const facebook::yoga::StyleSizeLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return resolve(length.callbackId(), percentRef);
  }

  return length.resolve(percentRef, nullptr).unwrap();
}

Float DynamicResolver::resolve(DynamicPropertyId id, float referenceLength)
    const {
  auto it = propertiesMap.find(id);
  if (it != propertiesMap.end()) {
    return it->second.resolve(
        referenceLength, context.viewportWidth(), context.viewportHeight());
  }
  return {};
}

Float DynamicResolver::resolve(
    const UntypedNumericValue& value,
    NumericValueDomain domain,
    float referenceLength) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !cssCalcMatchesDomain(it->second, domain)) {
      return std::numeric_limits<Float>::quiet_NaN();
    }
    return resolve(value.asDynamicId(), referenceLength);
  }

  if (!value.matchesDomain(domain)) {
    return std::numeric_limits<Float>::quiet_NaN();
  }

  return value.resolve(referenceLength);
}

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic DynamicResolver::toDynamicNumber(
    const UntypedNumericValue& value) const {
  return toDynamic(value, NumericValueDomain::Number);
}

folly::dynamic DynamicResolver::toDynamicLength(
    const UntypedNumericValue& value) const {
  return toDynamic(value, NumericValueDomain::Length);
}

folly::dynamic DynamicResolver::toDynamicLengthOrPercentage(
    const UntypedNumericValue& value,
    float percentRef) const {
  return toDynamic(value, NumericValueDomain::LengthOrPercentage, percentRef);
}

folly::dynamic DynamicResolver::toDynamic(
    const facebook::yoga::StyleLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return toDynamic(length.callbackId(), percentRef);
  }
  return {};
}
folly::dynamic DynamicResolver::toDynamic(
    const facebook::yoga::StyleSizeLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return toDynamic(length.callbackId(), percentRef);
  }
  return {};
}
folly::dynamic DynamicResolver::toDynamic(
    DynamicPropertyId id,
    float referenceLength) const {
  auto it = propertiesMap.find(id);
  if (it != propertiesMap.end()) {
    auto& calc = it->second;
    if (calc.isPercentOnly()) {
      return toString(calc.percent, '%');
    }
    return calc.resolve(
        referenceLength, context.viewportWidth(), context.viewportHeight());
  }
  return {};
}

folly::dynamic DynamicResolver::toDynamic(
    const UntypedNumericValue& value,
    NumericValueDomain domain,
    float referenceLength) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !cssCalcMatchesDomain(it->second, domain)) {
      return nullptr;
    }
    return toDynamic(value.asDynamicId(), referenceLength);
  }

  if (!value.matchesDomain(domain)) {
    return nullptr;
  }

  return value.toDynamic();
}
#endif

} // namespace facebook::react
