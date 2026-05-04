/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawValue.h>
#include <react/renderer/css/CSSCalc.h>
#include <react/renderer/css/CSSLength.h>
#include <react/renderer/css/CSSPercentage.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/NumericValue.h>

#include <optional>
#include <variant>

namespace facebook::react {

// ---------------------------------------------------------------------------
// parseNumericValue<T>
//
// Parse a RawValue into NumericValue<T>.  The concrete type T determines:
//   - which literal values are accepted (number vs. px vs. %)
//   - which TypedCalcEntry arm is inserted into DynamicPropertiesMap
// ---------------------------------------------------------------------------
template <NumericValueConcreteType T = NumericValueAny>
inline std::optional<NumericValue<T>> parseNumericValue(
    const PropsParserContext& context,
    const RawValue& value) {
  // Float literal
  if (value.hasType<Float>()) {
    auto numeric = static_cast<Float>(value);
    if constexpr (std::same_as<T, NumericValueNumber>) {
      return NumericValue<T>::number(numeric);
    } else if constexpr (SupportsLength<T>) {
      return NumericValue<T>::length(numeric);
    } else if constexpr (SupportsNumber<T>) {
      return NumericValue<T>::number(numeric);
    }
    return std::nullopt;
  }

  if (!value.hasType<std::string>()) {
    return std::nullopt;
  }

  auto stringValue = static_cast<std::string>(value);

  // px literal (for length-supporting types)
  if constexpr (SupportsLength<T>) {
    auto length = parseCSSProperty<CSSLength>(stringValue);
    if (std::holds_alternative<CSSLength>(length)) {
      auto cssLength = std::get<CSSLength>(length);
      if (cssLength.unit == CSSLengthUnit::Px) {
        return NumericValue<T>::length(cssLength.value);
      }
      return std::nullopt;
    }
  }

  // percentage literal (for percentage-supporting types, but not pure length)
  if constexpr (SupportsPercentage<T>) {
    auto pct = parseCSSProperty<CSSPercentage>(stringValue);
    if (std::holds_alternative<CSSPercentage>(pct)) {
      return NumericValue<T>::percentage(std::get<CSSPercentage>(pct).value);
    }
  }

  // calc()
  auto calc = parseCSSProperty<CSSCalc>(stringValue);
  if (!std::holds_alternative<CSSCalc>(calc)) {
    return std::nullopt;
  }

  auto cssCalc = std::get<CSSCalc>(calc);

  // Validate calc type matches T
  if constexpr (std::same_as<T, NumericValueNumber>) {
    if (!cssCalc.isUnitless()) {
      return std::nullopt;
    }
  } else if constexpr (std::same_as<T, NumericValueLength>) {
    if (cssCalc.isUnitless() || cssCalc.percent != 0.0f) {
      return std::nullopt;
    }
  } else if constexpr (SupportsLength<T> || SupportsPercentage<T>) {
    if (cssCalc.isUnitless()) {
      return std::nullopt;
    }
  }

  auto mapIt =
      context.contextContainer.find<std::shared_ptr<DynamicPropertiesMap>>(
          DynamicPropertiesMapKey);
  if (!mapIt) {
    return std::nullopt;
  }

  auto& map = *mapIt;
  auto index = map->allocateId();

  if constexpr (std::same_as<T, NumericValueNumber>) {
    map->insert_or_assign(index, NumberCalcEntry{cssCalc});
  } else if constexpr (std::same_as<T, NumericValueLength>) {
    map->insert_or_assign(index, LengthCalcEntry{cssCalc});
  } else {
    // LengthPercentage, Any → LengthOrPercentage entry
    map->insert_or_assign(index, LengthOrPercentageCalcEntry{cssCalc});
  }

  return NumericValue<T>::dynamic(index);
}

// ---------------------------------------------------------------------------
// parseNumericValueAs<T>
//
// Like parseNumericValue<T> but always returns UntypedNumericValue, which is
// useful when the result must be stored in a container that holds
// UntypedNumericValue (e.g. FilterFunction::parameters).
// ---------------------------------------------------------------------------
template <NumericValueConcreteType T>
inline std::optional<UntypedNumericValue> parseNumericValueAs(
    const PropsParserContext& context,
    const RawValue& value) {
  if (auto v = parseNumericValue<T>(context, value)) {
    return v->unwrap();
  }
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// fromRawValue<T>  (the generic prop-parsing hook)
// ---------------------------------------------------------------------------
template <NumericValueConcreteType T>
inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    NumericValue<T>& result) {
  if (auto parsed = parseNumericValue<T>(context, value)) {
    result = *parsed;
  }
}

// ---------------------------------------------------------------------------
// parseScaleNumericValue
//
// Parse a RawValue into an UntypedNumericValue for CSS scale() transform
// operations. Scale values are dimensionless:
//   - Float literals and unitless calc()  → treated as pure numbers.
//   - Percentage literals / percent-only calc() → divided by 100,
//     so "55%" or "calc(55%)" → number(0.55), matching CSS semantics where
//     scale(55%) means 55 % of identity scale (i.e. 0.55×).
//   - All other units (px, vw, vh …) → rejected (returns nullopt).
// ---------------------------------------------------------------------------
inline std::optional<UntypedNumericValue> parseScaleNumericValue(
    const PropsParserContext& context,
    const RawValue& value) {
  if (value.hasType<Float>()) {
    return UntypedNumericValue::number(static_cast<Float>(value));
  }
  if (!value.hasType<std::string>()) {
    return std::nullopt;
  }

  auto stringValue = static_cast<std::string>(value);

  // "55%" literal → number(0.55)
  auto pct = parseCSSProperty<CSSPercentage>(stringValue);
  if (std::holds_alternative<CSSPercentage>(pct)) {
    return UntypedNumericValue::number(
        std::get<CSSPercentage>(pct).value * 0.01f);
  }

  auto calc = parseCSSProperty<CSSCalc>(stringValue);
  if (!std::holds_alternative<CSSCalc>(calc)) {
    return std::nullopt;
  }
  auto cssCalc = std::get<CSSCalc>(calc);

  // calc(55%) → number(0.55)
  if (cssCalc.isPercentOnly()) {
    return UntypedNumericValue::number(cssCalc.percent * 0.01f);
  }

  // calc(0.55) → NumberCalcEntry (dynamic, for deferred resolution)
  if (!cssCalc.isUnitless()) {
    return std::nullopt; // reject calc(20px), calc(50vw), etc.
  }

  auto mapIt =
      context.contextContainer.find<std::shared_ptr<DynamicPropertiesMap>>(
          DynamicPropertiesMapKey);
  if (!mapIt) {
    return std::nullopt;
  }
  auto& map = *mapIt;
  auto index = map->allocateId();
  map->insert_or_assign(index, NumberCalcEntry{cssCalc});
  return UntypedNumericValue::dynamic(index);
}

// ---------------------------------------------------------------------------
// numericValueFromCSSCalc<T>
//
// Convert an already-parsed CSSCalc into NumericValue<T>.
// Simple values (unitless number, px-only length) are resolved immediately.
// Complex values (vw/vh/%) are stored in DynamicPropertiesMap for deferred
// viewport-relative resolution, exactly like parseNumericValue<T> does for
// calc() strings read from raw props.
// ---------------------------------------------------------------------------
template <NumericValueConcreteType T = NumericValueLength>
inline NumericValue<T> numericValueFromCSSCalc(
    const PropsParserContext& context,
    const CSSCalc& calc) {
  // Resolve immediately when no dynamic context is needed.
  if constexpr (std::same_as<T, NumericValueNumber>) {
    if (calc.isUnitless()) {
      return NumericValue<T>::number(calc.px);
    }
  } else if constexpr (std::same_as<T, NumericValueLength>) {
    if (!calc.isComplex() && calc.percent == 0.0f) {
      return NumericValue<T>::length(calc.px);
    }
  }

  // Complex calc — store in DynamicPropertiesMap for deferred resolution.
  auto mapIt =
      context.contextContainer.find<std::shared_ptr<DynamicPropertiesMap>>(
          DynamicPropertiesMapKey);
  if (!mapIt) {
    // No map available; degrade gracefully to the px component.
    if constexpr (SupportsLength<T>) {
      return NumericValue<T>::length(calc.px);
    } else if constexpr (SupportsNumber<T>) {
      return NumericValue<T>::number(calc.px);
    }
    return {};
  }

  auto& map = *mapIt;
  auto id = map->allocateId();
  if constexpr (std::same_as<T, NumericValueNumber>) {
    map->insert_or_assign(id, NumberCalcEntry{calc});
  } else if constexpr (std::same_as<T, NumericValueLength>) {
    map->insert_or_assign(id, LengthCalcEntry{calc});
  } else {
    // LengthPercentage, Any → LengthOrPercentage entry
    map->insert_or_assign(id, LengthOrPercentageCalcEntry{calc});
  }
  return NumericValue<T>::dynamic(id);
}

// ---------------------------------------------------------------------------
// numericValueFromCSSLengthPercentage
//
// Convert an already-parsed CSS length/percentage variant into an
// UntypedNumericValue. Used when the value has already gone through the CSS
// property parser (e.g. CSSTransform function arguments such as translateX)
// rather than being parsed fresh from a raw JS value.
// Only px lengths are accepted; other units return an empty value.
// ---------------------------------------------------------------------------
inline UntypedNumericValue numericValueFromCSSLengthPercentage(
    const std::variant<CSSLength, CSSPercentage>& value) {
  if (std::holds_alternative<CSSLength>(value)) {
    const auto& len = std::get<CSSLength>(value);
    if (len.unit != CSSLengthUnit::Px) {
      return {};
    }
    return UntypedNumericValue::length(len.value);
  }
  return UntypedNumericValue::percentage(std::get<CSSPercentage>(value).value);
}

} // namespace facebook::react
