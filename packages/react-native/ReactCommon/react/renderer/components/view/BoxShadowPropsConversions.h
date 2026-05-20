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
#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/core/propsConversions.h>
#include <react/renderer/css/CSSShadow.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/BoxShadow.h>
#include <react/utils/fnv1a.h>
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

inline BoxShadow fromCSSShadow(
    const PropsParserContext& /*context*/,
    const CSSShadow& cssShadow) {
  return BoxShadow{
      .offsetX = cssShadow.offsetX.resolve(0.0f, 0.0f, 0.0f),
      .offsetY = cssShadow.offsetY.resolve(0.0f, 0.0f, 0.0f),
      .blurRadius = cssShadow.blurRadius.resolve(0.0f, 0.0f, 0.0f),
      .spreadDistance = cssShadow.spreadDistance.resolve(0.0f, 0.0f, 0.0f),
      .color = fromCSSColor(cssShadow.color),
      .inset = cssShadow.inset,
  };
}

// Calc-aware variant: stores dynamic CSSCalc entries into DynamicPropertiesMap.
// path is expected to already be scoped to the shadow index (e.g.
// "boxShadow.0").
inline BoxShadow fromCSSShadowWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    const CSSShadow& cssShadow) {
  BoxShadow result;
  result.color = fromCSSColor(cssShadow.color);
  result.inset = cssShadow.inset;

  auto resolveField =
      [&](const CSSCalc& calc, const char* fieldName, Float& field) {
        DynamicPropertyPath::ScopedLevel level{path, fieldName};
        if (calc.isUnitless() || calc.isPointsOnly()) {
          field = calc.px;
        } else {
          if (context.calcMap) {
            context.calcMap->insert_or_assign(
                fnv1a(path.to_string()), NumberCalcEntry{calc});
          }
        }
      };

  resolveField(cssShadow.offsetX, "offsetX", result.offsetX);
  resolveField(cssShadow.offsetY, "offsetY", result.offsetY);
  resolveField(cssShadow.blurRadius, "blurRadius", result.blurRadius);
  resolveField(
      cssShadow.spreadDistance, "spreadDistance", result.spreadDistance);
  return result;
}

inline void parseUnprocessedBoxShadowString(
    const PropsParserContext& context,
    std::string&& value,
    std::vector<BoxShadow>& result) {
  auto boxShadowList = parseCSSProperty<CSSShadowList>((std::string)value);
  if (!std::holds_alternative<CSSShadowList>(boxShadowList)) {
    result = {};
    return;
  }

  for (const auto& cssShadow : std::get<CSSShadowList>(boxShadowList)) {
    result.push_back(fromCSSShadow(context, cssShadow));
  }
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

  if (!rawOffsetX->second.hasType<Float>()) {
    return {};
  }
  auto offsetX = (Float)rawOffsetX->second;

  auto rawOffsetY = boxShadow.find("offsetY");
  if (rawOffsetY == boxShadow.end()) {
    return {};
  }
  if (!rawOffsetY->second.hasType<Float>()) {
    return {};
  }
  auto offsetY = (Float)rawOffsetY->second;

  auto rawBlurRadius = boxShadow.find("blurRadius");
  Float blurRadius = 0.0f;
  if (rawBlurRadius != boxShadow.end()) {
    if (!rawBlurRadius->second.hasType<Float>()) {
      return {};
    }
    blurRadius = (Float)rawBlurRadius->second;
  }

  auto rawSpreadDistance = boxShadow.find("spreadDistance");
  Float spreadDistance = 0.0f;
  if (rawSpreadDistance != boxShadow.end()) {
    if (!rawSpreadDistance->second.hasType<Float>()) {
      return {};
    }
    spreadDistance = (Float)rawSpreadDistance->second;
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
      .offsetX = offsetX,
      .offsetY = offsetY,
      .blurRadius = blurRadius,
      .spreadDistance = spreadDistance,
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
    parseUnprocessedBoxShadowString(context, (std::string)value, result);
  } else if (value.hasType<std::vector<RawValue>>()) {
    parseUnprocessedBoxShadowList(
        context, (std::vector<RawValue>)value, result);
  } else {
    result = {};
  }
}

// ---------------------------------------------------------------------------
// Calc-aware parsing helpers
// ---------------------------------------------------------------------------

// Parses a single shadow object from a raw value map, using
// fromRawValueWithCalc for each numeric field so that dynamic calc()
// expressions are stored in DynamicPropertiesMap.
// path must already be scoped to the shadow's array index.
inline std::optional<BoxShadow> parseBoxShadowRawValueWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    const RawValue& value) {
  if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
    return {};
  }
  auto boxShadowMap = std::unordered_map<std::string, RawValue>(value);

  auto rawOffsetX = boxShadowMap.find("offsetX");
  if (rawOffsetX == boxShadowMap.end()) {
    return {};
  }
  Float offsetX = 0.0f;
  {
    DynamicPropertyPath::ScopedLevel level{path, "offsetX"};
    fromRawValueWithCalc(path, context, rawOffsetX->second, offsetX);
  }

  auto rawOffsetY = boxShadowMap.find("offsetY");
  if (rawOffsetY == boxShadowMap.end()) {
    return {};
  }
  Float offsetY = 0.0f;
  {
    DynamicPropertyPath::ScopedLevel level{path, "offsetY"};
    fromRawValueWithCalc(path, context, rawOffsetY->second, offsetY);
  }

  Float blurRadius = 0.0f;
  auto rawBlurRadius = boxShadowMap.find("blurRadius");
  if (rawBlurRadius != boxShadowMap.end()) {
    DynamicPropertyPath::ScopedLevel level{path, "blurRadius"};
    fromRawValueWithCalc(path, context, rawBlurRadius->second, blurRadius);
  }

  Float spreadDistance = 0.0f;
  auto rawSpreadDistance = boxShadowMap.find("spreadDistance");
  if (rawSpreadDistance != boxShadowMap.end()) {
    DynamicPropertyPath::ScopedLevel level{path, "spreadDistance"};
    fromRawValueWithCalc(
        path, context, rawSpreadDistance->second, spreadDistance);
  }

  bool inset = false;
  auto rawInset = boxShadowMap.find("inset");
  if (rawInset != boxShadowMap.end()) {
    if (rawInset->second.hasType<bool>()) {
      inset = (bool)rawInset->second;
    } else {
      return {};
    }
  }

  SharedColor color;
  auto rawColor = boxShadowMap.find("color");
  if (rawColor != boxShadowMap.end()) {
    color = coerceColor(rawColor->second, context);
    if (!color) {
      return {};
    }
  }

  return BoxShadow{
      .offsetX = offsetX,
      .offsetY = offsetY,
      .blurRadius = blurRadius,
      .spreadDistance = spreadDistance,
      .color = color,
      .inset = inset};
}

inline void parseProcessedBoxShadowWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }
  auto rawBoxShadows = static_cast<std::vector<RawValue>>(value);
  std::vector<BoxShadow> boxShadows{};
  for (size_t i = 0; i < rawBoxShadows.size(); ++i) {
    DynamicPropertyPath::ScopedLevel level{path, std::to_string(i)};
    if (auto bs =
            parseBoxShadowRawValueWithCalc(path, context, rawBoxShadows[i])) {
      boxShadows.push_back(*bs);
    } else {
      result = {};
      return;
    }
  }
  result = std::move(boxShadows);
}

inline void parseUnprocessedBoxShadowListWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    std::vector<RawValue>&& value,
    std::vector<BoxShadow>& result) {
  for (size_t i = 0; i < value.size(); ++i) {
    DynamicPropertyPath::ScopedLevel level{path, std::to_string(i)};
    if (auto bs = parseBoxShadowRawValueWithCalc(path, context, value[i])) {
      result.push_back(*bs);
    } else {
      result = {};
      return;
    }
  }
}

inline void parseUnprocessedBoxShadowStringWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    std::string&& value,
    std::vector<BoxShadow>& result) {
  auto boxShadowList = parseCSSProperty<CSSShadowList>(value);
  if (!std::holds_alternative<CSSShadowList>(boxShadowList)) {
    result = {};
    return;
  }
  size_t i = 0;
  for (const auto& cssShadow : std::get<CSSShadowList>(boxShadowList)) {
    DynamicPropertyPath::ScopedLevel level{path, std::to_string(i)};
    result.push_back(fromCSSShadowWithCalc(path, context, cssShadow));
    ++i;
  }
}

// Inserts the boxShadow sentinel into DynamicPropertiesMap so that
// needsToResolveStyleValues remains true whenever boxShadow is non-empty.
// This ensures getResolvedProps always re-serializes the parsed Float values
// over any rawProps string entries (e.g. "2px").
inline void insertBoxShadowSentinel(const PropsParserContext& context) {
  if (context.calcMap) {
    context.calcMap->insert_or_assign(
        fnv1a("boxShadow"), NumberCalcEntry{CSSCalc{}});
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  DynamicPropertyPath path;
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    if (value.hasType<std::string>()) {
      parseUnprocessedBoxShadowStringWithCalc(
          path, context, (std::string)value, result);
    } else if (value.hasType<std::vector<RawValue>>()) {
      parseUnprocessedBoxShadowListWithCalc(
          path, context, (std::vector<RawValue>)value, result);
    } else {
      result = {};
    }
  } else {
    parseProcessedBoxShadowWithCalc(path, context, value, result);
  }
  if (!result.empty()) {
    insertBoxShadowSentinel(context);
  }
}

inline void fromRawValueWithCalc(
    DynamicPropertyPath& path,
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BoxShadow>& result) {
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    if (value.hasType<std::string>()) {
      parseUnprocessedBoxShadowStringWithCalc(
          path, context, (std::string)value, result);
    } else if (value.hasType<std::vector<RawValue>>()) {
      parseUnprocessedBoxShadowListWithCalc(
          path, context, (std::vector<RawValue>)value, result);
    } else {
      result = {};
    }
  } else {
    parseProcessedBoxShadowWithCalc(path, context, value, result);
  }
  if (!result.empty()) {
    insertBoxShadowSentinel(context);
  }
}

} // namespace facebook::react
