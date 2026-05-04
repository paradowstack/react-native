/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <glog/logging.h>
#include <react/debug/react_native_expect.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/components/view/CSSConversions.h>
#include <react/renderer/components/view/NumericValueConversions.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/css/CSSFilter.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/Filter.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace facebook::react {

inline void parseProcessedFilter(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<FilterFunction>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  std::vector<FilterFunction> filter{};
  auto rawFilter = static_cast<std::vector<RawValue>>(value);
  for (const auto& rawFilterPrimitive : rawFilter) {
    bool isMap =
        rawFilterPrimitive.hasType<std::unordered_map<std::string, RawValue>>();
    react_native_expect(isMap);
    if (!isMap) {
      // If a filter is malformed then we should not apply any of them which
      // is the web behavior.
      result = {};
      return;
    }

    auto rawFilterFunction =
        static_cast<std::unordered_map<std::string, RawValue>>(
            rawFilterPrimitive);
    FilterFunction filterFunction{};
    auto filterType = filterTypeFromString(rawFilterFunction.begin()->first);
    if (!filterType.has_value()) {
      LOG(ERROR) << "Could not parse FilterFunction: "
                 << rawFilterFunction.begin()->first;
      result = {};
      return;
    }
    filterFunction.type = *filterType;
    if (filterFunction.type == FilterType::DropShadow) {
      auto rawDropShadow =
          static_cast<std::unordered_map<std::string, RawValue>>(
              rawFilterFunction.begin()->second);
      DropShadowParams dropShadowParams{};

      auto offsetX = rawDropShadow.find("offsetX");
      react_native_expect(offsetX != rawDropShadow.end());
      if (offsetX == rawDropShadow.end()) {
        result = {};
        return;
      }

      auto parsedOffsetX =
          parseNumericValueAs<NumericValueLength>(context, offsetX->second);
      if (!parsedOffsetX.has_value()) {
        result = {};
        return;
      }
      dropShadowParams.offsetX = *parsedOffsetX;

      auto offsetY = rawDropShadow.find("offsetY");
      react_native_expect(offsetY != rawDropShadow.end());
      if (offsetY == rawDropShadow.end()) {
        result = {};
        return;
      }
      auto parsedOffsetY =
          parseNumericValueAs<NumericValueLength>(context, offsetY->second);
      if (!parsedOffsetY.has_value()) {
        result = {};
        return;
      }
      dropShadowParams.offsetY = *parsedOffsetY;

      auto standardDeviation = rawDropShadow.find("standardDeviation");
      if (standardDeviation != rawDropShadow.end()) {
        auto parsedStandardDeviation = parseNumericValueAs<NumericValueLength>(
            context, standardDeviation->second);
        if (!parsedStandardDeviation.has_value()) {
          result = {};
          return;
        }
        dropShadowParams.standardDeviation = *parsedStandardDeviation;
      }

      auto color = rawDropShadow.find("color");
      if (color != rawDropShadow.end()) {
        fromRawValue(
            context.contextContainer,
            context.surfaceId,
            color->second,
            dropShadowParams.color);
      }

      filterFunction.parameters = dropShadowParams;
    } else {
      std::optional<UntypedNumericValue> parsedAmount =
          filterFunction.type == FilterType::Blur
          ? parseNumericValueAs<NumericValueLength>(
                context, rawFilterFunction.begin()->second)
          : parseNumericValueAs<NumericValueNumber>(
                context, rawFilterFunction.begin()->second);
      if (!parsedAmount.has_value()) {
        result = {};
        return;
      }
      filterFunction.parameters = *parsedAmount;
    }
    filter.push_back(filterFunction);
  }

  result = filter;
}

inline FilterType filterTypeFromVariant(const CSSFilterFunction& filter) {
  return std::visit(
      [](auto&& filter) -> FilterType {
        using FilterT = std::decay_t<decltype(filter)>;

        if constexpr (std::is_same_v<FilterT, CSSBlurFilter>) {
          return FilterType::Blur;
        }
        if constexpr (std::is_same_v<FilterT, CSSBrightnessFilter>) {
          return FilterType::Brightness;
        }
        if constexpr (std::is_same_v<FilterT, CSSContrastFilter>) {
          return FilterType::Contrast;
        }
        if constexpr (std::is_same_v<FilterT, CSSDropShadowFilter>) {
          return FilterType::DropShadow;
        }
        if constexpr (std::is_same_v<FilterT, CSSGrayscaleFilter>) {
          return FilterType::Grayscale;
        }
        if constexpr (std::is_same_v<FilterT, CSSHueRotateFilter>) {
          return FilterType::HueRotate;
        }
        if constexpr (std::is_same_v<FilterT, CSSInvertFilter>) {
          return FilterType::Invert;
        }
        if constexpr (std::is_same_v<FilterT, CSSOpacityFilter>) {
          return FilterType::Opacity;
        }
        if constexpr (std::is_same_v<FilterT, CSSSaturateFilter>) {
          return FilterType::Saturate;
        }
        if constexpr (std::is_same_v<FilterT, CSSSepiaFilter>) {
          return FilterType::Sepia;
        }
      },
      filter);
}

inline std::optional<FilterFunction> fromCSSFilter(
    const PropsParserContext& context,
    const CSSFilterFunction& cssFilter) {
  return std::visit(
      [&](auto&& filter) -> std::optional<FilterFunction> {
        using FilterT = std::decay_t<decltype(filter)>;

        if constexpr (std::is_same_v<FilterT, CSSBlurFilter>) {
          return FilterFunction{
              .type = filterTypeFromVariant(cssFilter),
              .parameters = numericValueFromCSSCalc<NumericValueLength>(
                                context, filter.amount)
                                .unwrap(),
          };
        }

        if constexpr (std::is_same_v<FilterT, CSSDropShadowFilter>) {
          return FilterFunction{
              .type = FilterType::DropShadow,
              .parameters = DropShadowParams{
                  .offsetX = numericValueFromCSSCalc<NumericValueLength>(
                      context, filter.offsetX),
                  .offsetY = numericValueFromCSSCalc<NumericValueLength>(
                      context, filter.offsetY),
                  .standardDeviation =
                      numericValueFromCSSCalc<NumericValueLength>(
                          context, filter.standardDeviation),
                  .color = fromCSSColor(filter.color),
              }};
        }

        if constexpr (
            std::is_same_v<FilterT, CSSBrightnessFilter> ||
            std::is_same_v<FilterT, CSSContrastFilter> ||
            std::is_same_v<FilterT, CSSGrayscaleFilter> ||
            std::is_same_v<FilterT, CSSInvertFilter> ||
            std::is_same_v<FilterT, CSSOpacityFilter> ||
            std::is_same_v<FilterT, CSSSaturateFilter> ||
            std::is_same_v<FilterT, CSSSepiaFilter>) {
          return FilterFunction{
              .type = filterTypeFromVariant(cssFilter),
              .parameters = UntypedNumericValue::number(filter.amount),
          };
        }

        if constexpr (std::is_same_v<FilterT, CSSHueRotateFilter>) {
          return FilterFunction{
              .type = filterTypeFromVariant(cssFilter),
              .parameters = UntypedNumericValue::number(filter.degrees),
          };
        }
      },
      cssFilter);
}

inline void parseUnprocessedFilterString(
    const PropsParserContext& context,
    std::string&& value,
    std::vector<FilterFunction>& result) {
  auto filterList = parseCSSProperty<CSSFilterList>((std::string)value);
  if (!std::holds_alternative<CSSFilterList>(filterList)) {
    result = {};
    return;
  }

  for (const auto& cssFilter : std::get<CSSFilterList>(filterList)) {
    if (auto filter = fromCSSFilter(context, cssFilter)) {
      result.push_back(*filter);
    } else {
      result = {};
      return;
    }
  }
}

inline std::optional<FilterFunction> parseDropShadow(
    const PropsParserContext& context,
    const RawValue& value) {
  if (value.hasType<std::string>()) {
    auto val = parseCSSProperty<CSSDropShadowFilter>(
        std::string("drop-shadow(") + (std::string)value + ")");
    if (std::holds_alternative<CSSDropShadowFilter>(val)) {
      return fromCSSFilter(context, std::get<CSSDropShadowFilter>(val));
    }
    return {};
  }

  if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
    return {};
  }
  auto rawDropShadow =
      static_cast<std::unordered_map<std::string, RawValue>>(value);

  DropShadowParams dropShadowParams{};

  auto offsetX = rawDropShadow.find("offsetX");
  if (offsetX == rawDropShadow.end()) {
    return {};
  }

  if (auto parsedOffsetX =
          parseNumericValueAs<NumericValueLength>(context, offsetX->second)) {
    dropShadowParams.offsetX = *parsedOffsetX;
  } else {
    return {};
  }

  auto offsetY = rawDropShadow.find("offsetY");
  if (offsetY == rawDropShadow.end()) {
    return {};
  }

  if (auto parsedOffsetY =
          parseNumericValueAs<NumericValueLength>(context, offsetY->second)) {
    dropShadowParams.offsetY = *parsedOffsetY;
  } else {
    return {};
  }

  auto standardDeviation = rawDropShadow.find("standardDeviation");
  if (standardDeviation != rawDropShadow.end()) {
    if (auto parsedStandardDeviation = parseNumericValueAs<NumericValueLength>(
            context, standardDeviation->second)) {
      if (!parsedStandardDeviation->isDynamic() &&
          parsedStandardDeviation->asFloat() < 0.0f) {
        return {};
      }
      dropShadowParams.standardDeviation = *parsedStandardDeviation;
    } else {
      return {};
    }
  }

  auto color = rawDropShadow.find("color");
  if (color != rawDropShadow.end()) {
    if (auto parsedColor = coerceColor(color->second, context)) {
      dropShadowParams.color = *parsedColor;
    } else {
      return {};
    }
  }

  return FilterFunction{
      .type = FilterType::DropShadow, .parameters = dropShadowParams};
}

inline std::optional<FilterFunction> parseFilterRawValue(
    const PropsParserContext& context,
    const RawValue& value) {
  if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
    return {};
  }
  auto rawFilter =
      static_cast<std::unordered_map<std::string, RawValue>>(value);

  if (rawFilter.size() != 1) {
    return {};
  }

  const auto& filterKey = rawFilter.begin()->first;

  if (filterKey == "drop-shadow") {
    return parseDropShadow(context, rawFilter.begin()->second);
  } else if (filterKey == "blur") {
    if (auto length = parseNumericValueAs<NumericValueLength>(
            context, rawFilter.begin()->second)) {
      if (!length->isDynamic() && length->asFloat() < 0.0f) {
        return {};
      }
      return FilterFunction{.type = FilterType::Blur, .parameters = *length};
    }
    return {};
  } else if (filterKey == "hue-rotate") {
    if (auto angle = coerceAngle(rawFilter.begin()->second)) {
      return FilterFunction{
          .type = FilterType::HueRotate,
          .parameters = UntypedNumericValue::number(*angle)};
    }
    return {};
  } else {
    auto filterType = filterTypeFromString(filterKey);
    if (!filterType.has_value()) {
      return {};
    }
    if (auto amount = parseNumericValueAs<NumericValueNumber>(
            context, rawFilter.begin()->second)) {
      if (!amount->isDynamic() && amount->asFloat() < 0.0f) {
        return {};
      }
      return FilterFunction{.type = *filterType, .parameters = *amount};
    }
    return {};
  }
}

inline void parseUnprocessedFilterList(
    const PropsParserContext& context,
    std::vector<RawValue>&& value,
    std::vector<FilterFunction>& result) {
  for (const auto& rawValue : value) {
    if (auto Filter = parseFilterRawValue(context, rawValue)) {
      result.push_back(*Filter);
    } else {
      result = {};
      return;
    }
  }
}

inline void parseUnprocessedFilter(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<FilterFunction>& result) {
  if (value.hasType<std::string>()) {
    parseUnprocessedFilterString(context, (std::string)value, result);
  } else if (value.hasType<std::vector<RawValue>>()) {
    parseUnprocessedFilterList(context, (std::vector<RawValue>)value, result);
  } else {
    result = {};
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<FilterFunction>& result) {
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    parseUnprocessedFilter(context, value, result);
  } else {
    parseProcessedFilter(context, value, result);
  }
}

} // namespace facebook::react
