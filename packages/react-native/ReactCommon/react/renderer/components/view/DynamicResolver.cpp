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

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

Float DynamicResolver::resolveCalcEntry(
    const TypedCalcEntry& entry,
    float referenceLength) const {
  return std::visit(
      [&](const auto& e) {
        return e.calc.resolve(
            referenceLength, context.viewportWidth(), context.viewportHeight());
      },
      entry);
}

Float DynamicResolver::resolveById(DynamicPropertyId id, float referenceLength)
    const {
  auto it = propertiesMap.find(id);
  if (it != propertiesMap.end()) {
    return resolveCalcEntry(it->second, referenceLength);
  }
  return {};
}

// ---------------------------------------------------------------------------
// Public resolve methods
// ---------------------------------------------------------------------------

Float DynamicResolver::resolveNumber(const UntypedNumericValue& value) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<NumberCalcEntry>(it->second)) {
      return std::numeric_limits<Float>::quiet_NaN();
    }
    return resolveCalcEntry(it->second, 0.0f);
  }
  if (value.kind() != NumericValueKind::Number) {
    return std::numeric_limits<Float>::quiet_NaN();
  }
  return value.resolve(0.0f);
}

Float DynamicResolver::resolveLength(const UntypedNumericValue& value) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<LengthCalcEntry>(it->second)) {
      return std::numeric_limits<Float>::quiet_NaN();
    }
    return resolveCalcEntry(it->second, 0.0f);
  }
  if (value.kind() != NumericValueKind::Length) {
    return std::numeric_limits<Float>::quiet_NaN();
  }
  return value.resolve(0.0f);
}

Float DynamicResolver::resolveLengthOrPercentage(
    const UntypedNumericValue& value,
    float percentRef) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<LengthOrPercentageCalcEntry>(it->second)) {
      return std::numeric_limits<Float>::quiet_NaN();
    }
    return resolveCalcEntry(it->second, percentRef);
  }
  auto k = value.kind();
  if (k != NumericValueKind::Length && k != NumericValueKind::Percentage) {
    return std::numeric_limits<Float>::quiet_NaN();
  }
  return value.resolve(percentRef);
}

LengthPercentageValue DynamicResolver::resolveLengthPercentage(
    const LengthPercentageValue& value,
    float percentRef) const {
  if (!value.isDynamic()) {
    return value;
  }

  auto it = propertiesMap.find(value.asDynamicId());
  if (it == propertiesMap.end()) {
    return LengthPercentageValue{};
  }

  return std::visit(
      [&](const auto& e) -> LengthPercentageValue {
        using E = std::decay_t<decltype(e)>;
        auto& calc = e.calc;
        if constexpr (std::is_same_v<E, LengthCalcEntry>) {
          auto resolved = calc.resolve(
              0.0f, context.viewportWidth(), context.viewportHeight());
          return LengthPercentageValue::length(resolved);
        } else if constexpr (std::is_same_v<E, LengthOrPercentageCalcEntry>) {
          if (calc.percent != 0.0f && percentRef <= 0.0f) {
            return value; // still dynamic – no layout yet
          }
          auto resolved = calc.resolve(
              percentRef, context.viewportWidth(), context.viewportHeight());
          if (calc.isPercentOnly()) {
            return LengthPercentageValue::percentage(resolved);
          }
          return LengthPercentageValue::length(resolved);
        } else {
          return LengthPercentageValue{};
        }
      },
      it->second);
}

UntypedNumericValue DynamicResolver::resolveAny(
    const UntypedNumericValue& value,
    float percentRef) const {
  if (!value.isDynamic()) {
    return value;
  }

  auto it = propertiesMap.find(value.asDynamicId());
  if (it == propertiesMap.end()) {
    return UntypedNumericValue{};
  }

  return std::visit(
      [&](const auto& e) -> UntypedNumericValue {
        using E = std::decay_t<decltype(e)>;
        auto& calc = e.calc;

        if constexpr (std::is_same_v<E, NumberCalcEntry>) {
          auto resolved = calc.resolve(
              0.0f, context.viewportWidth(), context.viewportHeight());
          return UntypedNumericValue::number(resolved);
        } else if constexpr (std::is_same_v<E, LengthCalcEntry>) {
          auto resolved = calc.resolve(
              0.0f, context.viewportWidth(), context.viewportHeight());
          return UntypedNumericValue::length(resolved);
        } else {
          // LengthOrPercentageCalcEntry
          if (calc.percent != 0.0f &&
              context.layoutMetrics.frame.size.width <= 0.0f &&
              context.layoutMetrics.frame.size.height <= 0.0f) {
            return value; // still dynamic – no layout yet
          }
          auto resolved = calc.resolve(
              percentRef, context.viewportWidth(), context.viewportHeight());
          if (calc.isPercentOnly()) {
            return UntypedNumericValue::percentage(resolved);
          }
          return UntypedNumericValue::length(resolved);
        }
      },
      it->second);
}

Float DynamicResolver::resolve(
    const facebook::yoga::StyleLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return resolveById(length.callbackId(), percentRef);
  }
  return length.resolve(percentRef, nullptr).unwrap();
}

Float DynamicResolver::resolve(
    const facebook::yoga::StyleSizeLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return resolveById(length.callbackId(), percentRef);
  }
  return length.resolve(percentRef, nullptr).unwrap();
}

// ---------------------------------------------------------------------------
// Serializable helpers
// ---------------------------------------------------------------------------

#ifdef RN_SERIALIZABLE_STATE

folly::dynamic DynamicResolver::toDynamicCalcEntry(
    const TypedCalcEntry& entry,
    float referenceLength) const {
  return std::visit(
      [&](const auto& e) -> folly::dynamic {
        using E = std::decay_t<decltype(e)>;
        auto& calc = e.calc;
        if constexpr (std::is_same_v<E, LengthOrPercentageCalcEntry>) {
          if (calc.isPercentOnly()) {
            return toString(calc.percent, '%');
          }
        }
        return calc.resolve(
            referenceLength, context.viewportWidth(), context.viewportHeight());
      },
      entry);
}

folly::dynamic DynamicResolver::toDynamicById(
    DynamicPropertyId id,
    float referenceLength) const {
  auto it = propertiesMap.find(id);
  if (it != propertiesMap.end()) {
    return toDynamicCalcEntry(it->second, referenceLength);
  }
  return {};
}

folly::dynamic DynamicResolver::toDynamicNumber(
    const UntypedNumericValue& value) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<NumberCalcEntry>(it->second)) {
      return nullptr;
    }
    return toDynamicById(value.asDynamicId(), 0.0f);
  }
  if (value.kind() != NumericValueKind::Number) {
    return nullptr;
  }
  return value.toDynamic();
}

folly::dynamic DynamicResolver::toDynamicLength(
    const UntypedNumericValue& value) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<LengthCalcEntry>(it->second)) {
      return nullptr;
    }
    return toDynamicById(value.asDynamicId(), 0.0f);
  }
  if (value.kind() != NumericValueKind::Length) {
    return nullptr;
  }
  return value.toDynamic();
}

folly::dynamic DynamicResolver::toDynamicLengthOrPercentage(
    const UntypedNumericValue& value,
    float percentRef) const {
  if (value.isDynamic()) {
    auto it = propertiesMap.find(value.asDynamicId());
    if (it == propertiesMap.end() ||
        !std::holds_alternative<LengthOrPercentageCalcEntry>(it->second)) {
      return nullptr;
    }
    return toDynamicById(value.asDynamicId(), percentRef);
  }
  auto k = value.kind();
  if (k != NumericValueKind::Length && k != NumericValueKind::Percentage) {
    return nullptr;
  }
  return value.toDynamic();
}

folly::dynamic DynamicResolver::toDynamicAny(
    const UntypedNumericValue& value,
    float percentRef) const {
  if (!value.isDynamic()) {
    return {};
  }
  return toDynamicById(value.asDynamicId(), percentRef);
}

folly::dynamic DynamicResolver::toDynamic(
    const facebook::yoga::StyleLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return toDynamicById(length.callbackId(), percentRef);
  }
  return {};
}

folly::dynamic DynamicResolver::toDynamic(
    const facebook::yoga::StyleSizeLength& length,
    float percentRef) const {
  if (length.isDynamic()) {
    return toDynamicById(length.callbackId(), percentRef);
  }
  return {};
}

#endif

} // namespace facebook::react
