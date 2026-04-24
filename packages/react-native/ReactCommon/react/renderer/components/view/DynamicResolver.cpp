/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "DynamicResolver.h"

#include <yoga/style/StyleLength.h>
#include <yoga/style/StyleSizeLength.h>

namespace facebook::react {

Float DynamicResolver::resolve(const FloatDynamic& value) const {
  if (value.isDynamic() && !propertiesMap.empty()) {
    return resolve(value.asDynamicId());
  }
  return value.value;
}

Float DynamicResolver::resolve(const ValueUnit& value, float percentRef) const {
  if (value.isDynamic()) {
    return resolve(value.asDynamicId(), percentRef);
  }

  return value.resolve(percentRef);
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

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic DynamicResolver::toDynamic(const FloatDynamic& value) const {
  if (value.isDynamic()) {
    return toDynamic(value.asDynamicId());
  }
  return value;
}
folly::dynamic DynamicResolver::toDynamic(
    const ValueUnit& value,
    float percentRef) const {
  if (value.isDynamic()) {
    return toDynamic(value.asDynamicId(), percentRef);
  }
  return value.toDynamic();
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
#endif

} // namespace facebook::react
