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
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/css/CSSShadow.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/BoxShadow.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace facebook::react {

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

    auto rawBoxShadowMap =
        static_cast<std::unordered_map<std::string, RawValue>>(rawBoxShadow);
    BoxShadow boxShadow{};
    // auto offsetX = rawBoxShadowMap.find("offsetX");
    // react_native_expect(offsetX != rawBoxShadowMap.end());
    // if (offsetX == rawBoxShadowMap.end()) {
    //   result = {};
    //   return;
    // }
    // react_native_expect(offsetX->second.hasType<Float>());
    // if (!offsetX->second.hasType<Float>()) {
    //   result = {};
    //   return;
    // }
    // boxShadow.offsetX = (Float)offsetX->second;

    // auto offsetY = rawBoxShadowMap.find("offsetY");
    // react_native_expect(offsetY != rawBoxShadowMap.end());
    // if (offsetY == rawBoxShadowMap.end()) {
    //   result = {};
    //   return;
    // }
    // react_native_expect(offsetY->second.hasType<Float>());
    // if (!offsetY->second.hasType<Float>()) {
    //   result = {};
    //   return;
    // }
    // boxShadow.offsetY = (Float)offsetY->second;

    auto mapIt =
        context.contextContainer.find<std::shared_ptr<CalcExpressions>>(
            CalcExpressionsKey);

    auto offsetX = rawBoxShadowMap.find("offsetX");
    if (offsetX != rawBoxShadowMap.end()) {
      // react_native_expect(offsetX->second.hasType<Float>());
      if (offsetX->second.hasType<Float>()) {
        boxShadow.offsetX = FloatDynamic{(Float)offsetX->second};
        // result = {};
        // return;
      } else if (offsetX->second.hasType<std::string>()) {
        auto offsetXStr = (std::string)offsetX->second;
        CSSSyntaxParser parser(offsetXStr);
        CSSValueParser valueParser(parser);
        auto parsedValue = valueParser.parseNextValue<CSSCalc>();
        if (auto cssCalc = std::get_if<CSSCalc>(&parsedValue)) {
          if (cssCalc->isPointsOnly()) {
            boxShadow.offsetX = FloatDynamic{cssCalc->px};
          } else if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert({index, *cssCalc});
            boxShadow.offsetX = FloatDynamic{index};
          } else {
            // We currently only support point values for blur radius, so if
            // it's not points then we should treat it as malformed
            result = {};
            return;
          }
        } else {
          // If it's not a valid CSS calc value then we should treat it as
          // malformed
          result = {};
          return;
        }
      } else {
        result = {};
        return;
      }
    }

    auto offsetY = rawBoxShadowMap.find("offsetY");
    if (offsetY != rawBoxShadowMap.end()) {
      // react_native_expect(offsetY->second.hasType<Float>());
      if (offsetY->second.hasType<Float>()) {
        boxShadow.offsetY = (Float)offsetY->second;
        // result = {};
        // return;
      } else if (offsetY->second.hasType<std::string>()) {
        auto offsetYStr = (std::string)offsetY->second;
        CSSSyntaxParser parser(offsetYStr);
        CSSValueParser valueParser(parser);
        auto parsedValue = valueParser.parseNextValue<CSSCalc>();
        if (auto cssCalc = std::get_if<CSSCalc>(&parsedValue)) {
          if (cssCalc->isPointsOnly()) {
            boxShadow.offsetY = cssCalc->px;
          } else if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert({index, *cssCalc});
            boxShadow.offsetY = FloatDynamic{index};
          } else {
            // We currently only support point values for blur radius, so if
            // it's not points then we should treat it as malformed
            result = {};
            return;
          }
        } else {
          // If it's not a valid CSS calc value then we should treat it as
          // malformed
          result = {};
          return;
        }
      } else {
        result = {};
        return;
      }
    }

    auto blurRadius = rawBoxShadowMap.find("blurRadius");
    if (blurRadius != rawBoxShadowMap.end()) {
      // react_native_expect(blurRadius->second.hasType<Float>());
      if (blurRadius->second.hasType<Float>()) {
        boxShadow.blurRadius = (Float)blurRadius->second;
        // result = {};
        // return;
      } else if (blurRadius->second.hasType<std::string>()) {
        auto blurRadiusStr = (std::string)blurRadius->second;
        CSSSyntaxParser parser(blurRadiusStr);
        CSSValueParser valueParser(parser);
        auto parsedValue = valueParser.parseNextValue<CSSCalc>();
        if (auto cssCalc = std::get_if<CSSCalc>(&parsedValue)) {
          if (cssCalc->isPointsOnly()) {
            boxShadow.blurRadius = cssCalc->px;
          } else if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert({index, *cssCalc});
          } else {
            // We currently only support point values for blur radius, so if
            // it's not points then we should treat it as malformed
            result = {};
            return;
          }
        } else {
          // If it's not a valid CSS calc value then we should treat it as
          // malformed
          result = {};
          return;
        }
      } else {
        result = {};
        return;
      }
    }

    auto spreadDistance = rawBoxShadowMap.find("spreadDistance");
    if (spreadDistance != rawBoxShadowMap.end()) {
      // react_native_expect(spreadDistance->second.hasType<Float>());
      if (spreadDistance->second.hasType<Float>()) {
        boxShadow.spreadDistance = (Float)spreadDistance->second;
        // result = {};
        // return;
      } else if (spreadDistance->second.hasType<std::string>()) {
        auto spreadDistanceStr = (std::string)spreadDistance->second;
        CSSSyntaxParser parser(spreadDistanceStr);
        CSSValueParser valueParser(parser);
        auto parsedValue = valueParser.parseNextValue<CSSCalc>();
        if (auto cssCalc = std::get_if<CSSCalc>(&parsedValue)) {
          if (cssCalc->isPointsOnly()) {
            boxShadow.spreadDistance = cssCalc->px;
          } else if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert({index, *cssCalc});
          } else {
            // We currently only support point values for blur radius, so if
            // it's not points then we should treat it as malformed
            result = {};
            return;
          }
        } else {
          // If it's not a valid CSS calc value then we should treat it as
          // malformed
          result = {};
          return;
        }
      } else {
        result = {};
        return;
      }
    }

    auto inset = rawBoxShadowMap.find("inset");
    if (inset != rawBoxShadowMap.end()) {
      react_native_expect(inset->second.hasType<bool>());
      if (!inset->second.hasType<bool>()) {
        result = {};
        return;
      }
      boxShadow.inset = (bool)inset->second;
    }

    auto color = rawBoxShadowMap.find("color");
    if (color != rawBoxShadowMap.end()) {
      fromRawValue(
          context.contextContainer,
          context.surfaceId,
          color->second,
          boxShadow.color);
    }

    boxShadows.push_back(boxShadow);
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
      .offsetX = FloatDynamic{cssShadow.offsetX.value},
      .offsetY = FloatDynamic{cssShadow.offsetY.value},
      .blurRadius = FloatDynamic{cssShadow.blurRadius.value},
      .spreadDistance = FloatDynamic{cssShadow.spreadDistance.value},
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

inline std::optional<FloatDynamic> toFloatDynamic(
    const PropsParserContext& context,
    const RawValue& value) {
  if (value.hasType<Float>()) {
    return FloatDynamic{(Float)value};
  }

  if (value.hasType<std::string>()) {
    auto len = parseCSSProperty<CSSLength>((std::string)value);
    if (std::holds_alternative<CSSLength>(len)) {
      auto cssLen = std::get<CSSLength>(len);
      if (cssLen.unit == CSSLengthUnit::Px) {
        return FloatDynamic{(Float)cssLen.value};
      }
    }

    auto calc = parseCSSProperty<CSSCalc>((std::string)value);
    if (std::holds_alternative<CSSCalc>(calc)) {
      auto cssCalc = std::get<CSSCalc>(calc);
      if (cssCalc.isUnitless() || cssCalc.isPointsOnly()) {
        return FloatDynamic{(Float)cssCalc.px};
      }
      if (cssCalc.isComplex()) {
        if (auto mapIt =
                context.contextContainer.find<std::shared_ptr<CalcExpressions>>(
                    CalcExpressionsKey)) {
          auto& map = *mapIt;
          auto index = map->allocateId();
          map->insert({index, cssCalc});
          return FloatDynamic{index};
        }
      }
    }
  }
  return {};
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

  auto offsetX = toFloatDynamic(context, rawOffsetX->second);
  if (!offsetX.has_value()) {
    return {};
  }

  auto rawOffsetY = boxShadow.find("offsetY");
  if (rawOffsetY == boxShadow.end()) {
    return {};
  }
  auto offsetY = toFloatDynamic(context, rawOffsetY->second);
  if (!offsetY.has_value()) {
    return {};
  }

  auto rawBlurRadius = boxShadow.find("blurRadius");
  auto blurRadius = toFloatDynamic(context, rawBlurRadius->second);
  if (!blurRadius.has_value()) {
    return {};
  }

  auto rawSpreadDistance = boxShadow.find("spreadDistance");
  auto spreadDistance = toFloatDynamic(context, rawSpreadDistance->second);
  if (!spreadDistance.has_value()) {
    return {};
  }

  if (rawSpreadDistance != boxShadow.end()) {
    if (auto spreadDistanceValue = coerceLength(rawSpreadDistance->second)) {
      spreadDistance = *spreadDistanceValue;
    } else {
      return {};
    }
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
