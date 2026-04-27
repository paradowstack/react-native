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

namespace facebook::react {

inline bool cssCalcMatchesDomain(
    const CSSCalc& calc,
    NumericValueDomain domain) {
  switch (domain) {
    case NumericValueDomain::Number:
      return calc.isUnitless();
    case NumericValueDomain::Length:
      return !calc.isUnitless() && calc.percent == 0.0f;
    case NumericValueDomain::LengthOrPercentage:
      return !calc.isUnitless();
  }
}

inline std::optional<UntypedNumericValue> parseNumericValue(
    const PropsParserContext& context,
    const RawValue& value,
    NumericValueDomain domain) {
  if (value.hasType<Float>()) {
    auto numeric = static_cast<Float>(value);
    return domain == NumericValueDomain::Number
        ? UntypedNumericValue::number(numeric)
        : UntypedNumericValue::length(numeric);
  }

  if (!value.hasType<std::string>()) {
    return std::nullopt;
  }

  auto stringValue = static_cast<std::string>(value);
  if (domain != NumericValueDomain::Number) {
    auto length = parseCSSProperty<CSSLength>(stringValue);
    if (std::holds_alternative<CSSLength>(length)) {
      auto cssLength = std::get<CSSLength>(length);
      if (cssLength.unit == CSSLengthUnit::Px) {
        return UntypedNumericValue::length(cssLength.value);
      }
      return std::nullopt;
    }
  }

  if (domain == NumericValueDomain::LengthOrPercentage) {
    auto percentage = parseCSSProperty<CSSPercentage>(stringValue);
    if (std::holds_alternative<CSSPercentage>(percentage)) {
      return UntypedNumericValue::percentage(
          std::get<CSSPercentage>(percentage).value);
    }
  }

  auto calc = parseCSSProperty<CSSCalc>(stringValue);
  if (!std::holds_alternative<CSSCalc>(calc)) {
    return std::nullopt;
  }

  auto cssCalc = std::get<CSSCalc>(calc);
  if (!cssCalcMatchesDomain(cssCalc, domain)) {
    return std::nullopt;
  }

  auto mapIt =
      context.contextContainer.find<std::shared_ptr<DynamicPropertiesMap>>(
          DynamicPropertiesMapKey);
  if (!mapIt) {
    return std::nullopt;
  }

  auto& map = *mapIt;
  auto index = map->allocateId();
  map->insert_or_assign(index, cssCalc);
  return UntypedNumericValue::dynamic(index, domain);
}

template <typename DomainT>
inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    NumericValue<DomainT>& result) {
  if (auto parsed = parseNumericValue(context, value, result.domain())) {
    result = *parsed;
  }
}

} // namespace facebook::react
