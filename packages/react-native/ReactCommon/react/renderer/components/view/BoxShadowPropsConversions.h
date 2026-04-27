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
#include <react/renderer/css/CSSShadow.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/BoxShadow.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace facebook::react {

inline std::optional<BoxShadow> parseBoxShadowRawValue(
    const PropsParserContext& context,
    const RawValue& value);

inline void parseProcessedBoxShadow(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  std::vector<BoxShadow> boxShadows{};
  auto rawBoxShadows = static_cast<std::vector<RawValue>>(value);
  for (const auto& rawBoxShadow : rawBoxShadows) {
    bool isMap =
        rawBoxShadow.hasType<std::unordered_map<std::string, RawValue>>();
    react_native_expect(isMap);
    if (!isMap) {
      // If any box shadow is malformed then we should not apply any of them
      // which is the web behavior.
      result = {};
      return;
    }

    if (auto boxShadow = parseBoxShadowRawValue(context, rawBoxShadow)) {
      boxShadows.push_back(*boxShadow);
    } else {
      result = {};
      return;
    }
  }

  result = boxShadows;
}

inline std::optional<BoxShadow> fromCSSShadow(const CSSShadow& cssShadow) {
  // TODO: handle non-px values
  if (cssShadow.offsetX.unit != CSSLengthUnit::Px ||
      cssShadow.offsetY.unit != CSSLengthUnit::Px ||
      cssShadow.blurRadius.unit != CSSLengthUnit::Px ||
      cssShadow.spreadDistance.unit != CSSLengthUnit::Px) {
    return {};
  }

  return BoxShadow{
      .offsetX = LengthValue::length(cssShadow.offsetX.value),
      .offsetY = LengthValue::length(cssShadow.offsetY.value),
      .blurRadius = LengthValue::length(cssShadow.blurRadius.value),
      .spreadDistance = LengthValue::length(cssShadow.spreadDistance.value),
      .color = fromCSSColor(cssShadow.color),
      .inset = cssShadow.inset,
  };
}

inline void parseUnprocessedBoxShadowString(
    std::string&& value,
    std::vector<BoxShadow>& result) {
  auto boxShadowList = parseCSSProperty<CSSShadowList>((std::string)value);
  if (!std::holds_alternative<CSSShadowList>(boxShadowList)) {
    result = {};
    return;
  }

  for (const auto& cssShadow : std::get<CSSShadowList>(boxShadowList)) {
    if (auto boxShadow = fromCSSShadow(cssShadow)) {
      result.push_back(*boxShadow);
    } else {
      result = {};
      return;
    }
  }
}

inline std::optional<LengthValue> toLengthNumericValue(
    const PropsParserContext& context,
    const RawValue& value) {
  return parseNumericValue(context, value, NumericValueDomain::Length);
}

inline std::optional<BoxShadow> parseBoxShadowRawValue(
    const PropsParserContext& context,
    const RawValue& value) {
  if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
    return {};
  }

  auto boxShadow = std::unordered_map<std::string, RawValue>(value);
  auto rawOffsetX = boxShadow.find("offsetX");
  if (rawOffsetX == boxShadow.end()) {
    return {};
  }

  auto offsetX = toLengthNumericValue(context, rawOffsetX->second);
  if (!offsetX.has_value()) {
    return {};
  }

  auto rawOffsetY = boxShadow.find("offsetY");
  if (rawOffsetY == boxShadow.end()) {
    return {};
  }
  auto offsetY = toLengthNumericValue(context, rawOffsetY->second);
  if (!offsetY.has_value()) {
    return {};
  }

  auto rawBlurRadius = boxShadow.find("blurRadius");
  auto blurRadius = rawBlurRadius != boxShadow.end()
      ? toLengthNumericValue(context, rawBlurRadius->second)
      : std::optional<LengthValue>(LengthValue::length(0.0f));
  if (!blurRadius.has_value()) {
    return {};
  }

  auto rawSpreadDistance = boxShadow.find("spreadDistance");
  auto spreadDistance = rawSpreadDistance != boxShadow.end()
      ? toLengthNumericValue(context, rawSpreadDistance->second)
      : std::optional<LengthValue>(LengthValue::length(0.0f));
  if (!spreadDistance.has_value()) {
    return {};
  }

  bool inset = false;
  auto rawInset = boxShadow.find("inset");
  if (rawInset != boxShadow.end()) {
    if (rawInset->second.hasType<bool>()) {
      inset = (bool)rawInset->second;
    } else {
      return {};
    }
  }

  SharedColor color;
  auto rawColor = boxShadow.find("color");
  if (rawColor != boxShadow.end()) {
    color = coerceColor(rawColor->second, context);
    if (!color) {
      return {};
    }
  }

  return BoxShadow{
      .offsetX = *offsetX,
      .offsetY = *offsetY,
      .blurRadius = *blurRadius,
      .spreadDistance = *spreadDistance,
      .color = color,
      .inset = inset};
}

inline void parseUnprocessedBoxShadowList(
    const PropsParserContext& context,
    std::vector<RawValue>&& value,
    std::vector<BoxShadow>& result) {
  for (const auto& rawValue : value) {
    if (auto boxShadow = parseBoxShadowRawValue(context, rawValue)) {
      result.push_back(*boxShadow);
    } else {
      result = {};
      return;
    }
  }
}

inline void parseUnprocessedBoxShadow(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  if (value.hasType<std::string>()) {
    parseUnprocessedBoxShadowString((std::string)value, result);
  } else if (value.hasType<std::vector<RawValue>>()) {
    parseUnprocessedBoxShadowList(
        context, (std::vector<RawValue>)value, result);
  } else {
    result = {};
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    parseUnprocessedBoxShadow(context, value, result);
  } else {
    parseProcessedBoxShadow(context, value, result);
  }
}

} // namespace facebook::react
