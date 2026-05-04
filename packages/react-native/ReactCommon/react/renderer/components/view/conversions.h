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
#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/components/view/NumericValueConversions.h>
#include <react/renderer/components/view/primitives.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/core/graphicsConversions.h>
#include <react/renderer/css/CSSAngle.h>
#include <react/renderer/css/CSSPercentage.h>
#include <react/renderer/css/CSSRatio.h>
#include <react/renderer/css/CSSTransform.h>
#include <react/renderer/css/CSSTransformOrigin.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/debug/DebugStringConvertible.h>
#include <react/renderer/debug/flags.h>
#include <react/renderer/graphics/BackgroundPosition.h>
#include <react/renderer/graphics/BackgroundRepeat.h>
#include <react/renderer/graphics/BackgroundSize.h>
#include <react/renderer/graphics/BlendMode.h>
#include <react/renderer/graphics/Isolation.h>
#include <react/renderer/graphics/LinearGradient.h>
#include <react/renderer/graphics/PlatformColorParser.h>
#include <react/renderer/graphics/Transform.h>
#include <yoga/YGEnums.h>
#include <yoga/node/Node.h>
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>

namespace facebook::react {

// Yoga calc() value resolver — defined in YogaStylableProps.cpp,
// called by Yoga during layout to resolve dynamic style values.
YGValue yogaNodeCalcValueResolver(
    YGNodeConstRef yogaNode,
    YGValueDynamicID id,
    YGValueDynamicContext context);

/*
 * Yoga's `float` <-> React Native's `Float` (can be `double` or `float`)
 *
 * Regular Yoga `float` values represent some onscreen-position-related values.
 * They can be real numbers or special value `YGUndefined` (which actually is
 * `NaN`). Conceptually, layout computation process inside Yoga should never
 * produce `NaN` values from non-`NaN` values. At the same time, ` YGUndefined`
 * values have special "no limit" meaning in Yoga, therefore ` YGUndefined`
 * usually corresponds to `Infinity` value.
 */
inline Float floatFromYogaFloat(float value) {
  static_assert(
      YGUndefined != YGUndefined,
      "The code of this function assumes that YGUndefined is NaN.");
  if (std::isnan(value) /* means: `value == YGUndefined` */) {
    return std::numeric_limits<Float>::infinity();
  }

  return (Float)value;
}

inline float yogaFloatFromFloat(Float value) {
  if (!std::isfinite(value)) {
    return YGUndefined;
  }

  return (float)value;
}

/*
 * `yoga::FloatOptional` <-> React Native's `Float`
 *
 * `yoga::FloatOptional` represents optional dimensionless float values in Yoga
 * Style object (e.g. `flex`). The most suitable analogy to empty
 * `yoga::FloatOptional` is `NaN` value.
 * `yoga::FloatOptional` values are usually parsed from some outside data source
 * which usually has some special corresponding representation for an empty
 * value.
 */
inline Float floatFromYogaOptionalFloat(yoga::FloatOptional value) {
  if (value.isUndefined()) {
    return std::numeric_limits<Float>::quiet_NaN();
  }

  return floatFromYogaFloat(value.unwrap());
}

inline yoga::FloatOptional yogaOptionalFloatFromFloat(Float value) {
  if (std::isnan(value)) {
    return yoga::FloatOptional();
  }

  return yoga::FloatOptional((float)value);
}

inline std::optional<Float> optionalFloatFromYogaValue(
    const yoga::Style::Length& length,
    std::optional<Float> base = {}) {
  if (length.isPoints()) {
    return floatFromYogaOptionalFloat(length.value());
  } else if (length.isPercent()) {
    return base.has_value()
        ? std::optional<Float>(
              base.value() * floatFromYogaOptionalFloat(length.value()))
        : std::optional<Float>();
  } else {
    return {};
  }
}

static inline PositionType positionTypeFromYogaPositionType(
    yoga::PositionType positionType) {
  switch (positionType) {
    case yoga::PositionType::Static:
      return PositionType::Static;
    case yoga::PositionType::Relative:
      return PositionType::Relative;
    case yoga::PositionType::Absolute:
      return PositionType::Absolute;
  }
}

inline DisplayType displayTypeFromYGDisplay(YGDisplay display) {
  switch (display) {
    case YGDisplayNone:
      return DisplayType::None;
    case YGDisplayContents:
      return DisplayType::Contents;
    case YGDisplayFlex:
      return DisplayType::Flex;
    case YGDisplayGrid:
      return DisplayType::Grid;
  }
}

inline LayoutMetrics layoutMetricsFromYogaNode(yoga::Node& yogaNode) {
  auto layoutMetrics = LayoutMetrics{};

  layoutMetrics.frame = Rect{
      .origin =
          Point{
              .x = floatFromYogaFloat(YGNodeLayoutGetLeft(&yogaNode)),
              .y = floatFromYogaFloat(YGNodeLayoutGetTop(&yogaNode))},
      .size = Size{
          .width = floatFromYogaFloat(YGNodeLayoutGetWidth(&yogaNode)),
          .height = floatFromYogaFloat(YGNodeLayoutGetHeight(&yogaNode))}};

  layoutMetrics.borderWidth = EdgeInsets{
      floatFromYogaFloat(YGNodeLayoutGetBorder(&yogaNode, YGEdgeLeft)),
      floatFromYogaFloat(YGNodeLayoutGetBorder(&yogaNode, YGEdgeTop)),
      floatFromYogaFloat(YGNodeLayoutGetBorder(&yogaNode, YGEdgeRight)),
      floatFromYogaFloat(YGNodeLayoutGetBorder(&yogaNode, YGEdgeBottom))};

  layoutMetrics.contentInsets = EdgeInsets{
      layoutMetrics.borderWidth.left +
          floatFromYogaFloat(YGNodeLayoutGetPadding(&yogaNode, YGEdgeLeft)),
      layoutMetrics.borderWidth.top +
          floatFromYogaFloat(YGNodeLayoutGetPadding(&yogaNode, YGEdgeTop)),
      layoutMetrics.borderWidth.right +
          floatFromYogaFloat(YGNodeLayoutGetPadding(&yogaNode, YGEdgeRight)),
      layoutMetrics.borderWidth.bottom +
          floatFromYogaFloat(YGNodeLayoutGetPadding(&yogaNode, YGEdgeBottom))};

  layoutMetrics.displayType =
      displayTypeFromYGDisplay(YGNodeStyleGetDisplay(&yogaNode));

  layoutMetrics.positionType =
      positionTypeFromYogaPositionType(yogaNode.style().positionType());

  layoutMetrics.layoutDirection =
      YGNodeLayoutGetDirection(&yogaNode) == YGDirectionRTL
      ? LayoutDirection::RightToLeft
      : LayoutDirection::LeftToRight;

  return layoutMetrics;
}

inline YGDirection yogaDirectionFromLayoutDirection(LayoutDirection direction) {
  switch (direction) {
    case LayoutDirection::Undefined:
      return YGDirectionInherit;
    case LayoutDirection::LeftToRight:
      return YGDirectionLTR;
    case LayoutDirection::RightToLeft:
      return YGDirectionRTL;
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Direction& result) {
  result = yoga::Direction::Inherit;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "inherit") {
    result = yoga::Direction::Inherit;
    return;
  }
  if (stringValue == "ltr") {
    result = yoga::Direction::LTR;
    return;
  }
  if (stringValue == "rtl") {
    result = yoga::Direction::RTL;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Direction: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::FlexDirection& result) {
  result = yoga::FlexDirection::Column;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "row") {
    result = yoga::FlexDirection::Row;
    return;
  }
  if (stringValue == "column") {
    result = yoga::FlexDirection::Column;
    return;
  }
  if (stringValue == "column-reverse") {
    result = yoga::FlexDirection::ColumnReverse;
    return;
  }
  if (stringValue == "row-reverse") {
    result = yoga::FlexDirection::RowReverse;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::FlexDirection: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    yoga::BoxSizing& result) {
  result = yoga::BoxSizing::BorderBox;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "border-box") {
    result = yoga::BoxSizing::BorderBox;
    return;
  }
  if (stringValue == "content-box") {
    result = yoga::BoxSizing::ContentBox;
    return;
  }

  LOG(ERROR) << "Could not parse yoga::BoxSizing: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Justify& result) {
  result = yoga::Justify::FlexStart;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "flex-start") {
    result = yoga::Justify::FlexStart;
    return;
  }
  if (stringValue == "center") {
    result = yoga::Justify::Center;
    return;
  }
  if (stringValue == "flex-end") {
    result = yoga::Justify::FlexEnd;
    return;
  }
  if (stringValue == "space-between") {
    result = yoga::Justify::SpaceBetween;
    return;
  }
  if (stringValue == "space-around") {
    result = yoga::Justify::SpaceAround;
    return;
  }
  if (stringValue == "space-evenly") {
    result = yoga::Justify::SpaceEvenly;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Justify: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Align& result) {
  result = yoga::Align::Stretch;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "auto") {
    result = yoga::Align::Auto;
    return;
  }
  if (stringValue == "flex-start") {
    result = yoga::Align::FlexStart;
    return;
  }
  if (stringValue == "center") {
    result = yoga::Align::Center;
    return;
  }
  if (stringValue == "flex-end") {
    result = yoga::Align::FlexEnd;
    return;
  }
  if (stringValue == "stretch") {
    result = yoga::Align::Stretch;
    return;
  }
  if (stringValue == "baseline") {
    result = yoga::Align::Baseline;
    return;
  }
  if (stringValue == "space-between") {
    result = yoga::Align::SpaceBetween;
    return;
  }
  if (stringValue == "space-around") {
    result = yoga::Align::SpaceAround;
    return;
  }
  if (stringValue == "space-evenly") {
    result = yoga::Align::SpaceEvenly;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Align: " << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::PositionType& result) {
  result = yoga::PositionType::Relative;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "static") {
    result = yoga::PositionType::Static;
    return;
  }
  if (stringValue == "relative") {
    result = yoga::PositionType::Relative;
    return;
  }
  if (stringValue == "absolute") {
    result = yoga::PositionType::Absolute;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::PositionType: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Wrap& result) {
  result = yoga::Wrap::NoWrap;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "nowrap") {
    result = yoga::Wrap::NoWrap;
    return;
  }
  if (stringValue == "wrap") {
    result = yoga::Wrap::Wrap;
    return;
  }
  if (stringValue == "wrap-reverse") {
    result = yoga::Wrap::WrapReverse;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Wrap: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Overflow& result) {
  result = yoga::Overflow::Visible;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "visible") {
    result = yoga::Overflow::Visible;
    return;
  }
  if (stringValue == "hidden") {
    result = yoga::Overflow::Hidden;
    return;
  }
  if (stringValue == "scroll") {
    result = yoga::Overflow::Scroll;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Overflow:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Display& result) {
  result = yoga::Display::Flex;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "flex") {
    result = yoga::Display::Flex;
    return;
  }
  if (stringValue == "none") {
    result = yoga::Display::None;
    return;
  }
  if (stringValue == "contents") {
    result = yoga::Display::Contents;
    return;
  }
  LOG(ERROR) << "Could not parse yoga::Display: " << stringValue;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Style::SizeLength& result) {
  if (value.hasType<Float>()) {
    result = yoga::StyleSizeLength::points((float)value);
    return;
  } else if (value.hasType<std::string>()) {
    const auto stringValue = (std::string)value;
    if (stringValue == "auto") {
      result = yoga::StyleSizeLength::ofAuto();
      return;
    } else if (stringValue == "max-content") {
      result = yoga::StyleSizeLength::ofMaxContent();
      return;
    } else if (stringValue == "stretch") {
      result = yoga::StyleSizeLength::ofStretch();
      return;
    } else if (stringValue == "fit-content") {
      result = yoga::StyleSizeLength::ofFitContent();
      return;
    } else {
      auto parsed =
          parseCSSProperty<CSSCalc, CSSNumber, CSSPercentage>(stringValue);
      if (std::holds_alternative<CSSPercentage>(parsed)) {
        result = yoga::StyleSizeLength::percent(
            std::get<CSSPercentage>(parsed).value);
        return;
      } else if (std::holds_alternative<CSSNumber>(parsed)) {
        result =
            yoga::StyleSizeLength::points(std::get<CSSNumber>(parsed).value);
        return;
      } else if (std::holds_alternative<CSSCalc>(parsed)) {
        auto cssCalc = std::get<CSSCalc>(parsed);
        if (cssCalc.isPointsOnly()) {
          result = yoga::StyleSizeLength::points(cssCalc.px);
          return;
        }
        if (cssCalc.isPercentOnly()) {
          result = yoga::StyleSizeLength::percent(cssCalc.percent);
          return;
        }
        if (cssCalc.isComplex()) {
          auto mapIt = context.contextContainer
                           .find<std::shared_ptr<DynamicPropertiesMap>>(
                               DynamicPropertiesMapKey);
          if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert_or_assign(index, LengthOrPercentageCalcEntry{cssCalc});
            result = yoga::StyleSizeLength::dynamic(
                &yogaNodeCalcValueResolver, index);
            return;
          }
        }
      }
    }
  }
  result = yoga::StyleSizeLength::undefined();
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::Style::Length& result) {
  if (value.hasType<Float>()) {
    result = yoga::StyleLength::points((float)value);
    return;
  } else if (value.hasType<std::string>()) {
    const auto stringValue = (std::string)value;
    if (stringValue == "auto") {
      result = yoga::StyleLength::ofAuto();
      return;
    } else {
      auto parsed =
          parseCSSProperty<CSSCalc, CSSNumber, CSSPercentage>(stringValue);
      if (std::holds_alternative<CSSPercentage>(parsed)) {
        result =
            yoga::StyleLength::percent(std::get<CSSPercentage>(parsed).value);
        return;
      } else if (std::holds_alternative<CSSNumber>(parsed)) {
        result = yoga::StyleLength::points(std::get<CSSNumber>(parsed).value);
        return;
      } else if (std::holds_alternative<CSSCalc>(parsed)) {
        auto cssCalc = std::get<CSSCalc>(parsed);
        if (cssCalc.isPointsOnly()) {
          result = yoga::StyleLength::points(cssCalc.px);
          return;
        }
        if (cssCalc.isPercentOnly()) {
          result = yoga::StyleLength::percent(cssCalc.percent);
          return;
        }
        if (cssCalc.isComplex()) {
          auto mapIt = context.contextContainer
                           .find<std::shared_ptr<DynamicPropertiesMap>>(
                               DynamicPropertiesMapKey);
          if (mapIt) {
            auto& map = *mapIt;
            auto index = map->allocateId();
            map->insert_or_assign(index, LengthOrPercentageCalcEntry{cssCalc});
            result =
                yoga::StyleLength::dynamic(&yogaNodeCalcValueResolver, index);
            return;
          }
        }
      }
    }
  }
  result = yoga::StyleLength::undefined();
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    YGValue& result) {
  yoga::Style::Length length{};
  fromRawValue(context, value, length);
  result = (YGValue)length;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    yoga::FloatOptional& result) {
  result = value.hasType<float>() ? yoga::FloatOptional((float)value)
                                  : yoga::FloatOptional();
}

inline yoga::FloatOptional convertAspectRatio(
    const PropsParserContext& /*context*/,
    const RawValue& value) {
  if (value.hasType<float>()) {
    return yoga::FloatOptional((float)value);
  }
  if (ReactNativeFeatureFlags::enableNativeCSSParsing() &&
      value.hasType<std::string>()) {
    auto ratio = parseCSSProperty<CSSRatio>((std::string)value);
    if (std::holds_alternative<CSSRatio>(ratio)) {
      auto r = std::get<CSSRatio>(ratio);
      if (!r.isDegenerate()) {
        return yoga::FloatOptional(r.numerator / r.denominator);
      }
    }
  }
  return {};
}

inline std::optional<Float> toRadians(const RawValue& value) {
  if (value.hasType<Float>()) {
    return (Float)value;
  }
  if (!value.hasType<std::string>()) {
    return {};
  }

  auto angle = parseCSSProperty<CSSAngle>((std::string)value);
  if (std::holds_alternative<CSSAngle>(angle)) {
    return static_cast<float>(
        std::get<CSSAngle>(angle).degrees * M_PI / 180.0f);
  }

  return {};
}

// `Float` is handled by the explicit specialization in
// react/renderer/core/propsConversions.h (parse number + calc() strings).
// Do not add a non-template overload here — it would win overload resolution
// and break string/calc parsing for opacity and other Float props.

inline std::optional<TransformOperation> fromCSSTransformFunction(
    const CSSTransformFunction& cssTransform) {
  constexpr auto ZeroNumber = UntypedNumericValue::number(0.0f);
  constexpr auto ZeroLength = UntypedNumericValue::length(0.0f);
  constexpr auto One = UntypedNumericValue::number(1.0f);

  return std::visit(
      [&](auto&& func) -> std::optional<TransformOperation> {
        using T = std::decay_t<decltype(func)>;

        if constexpr (std::is_same_v<T, CSSRotate>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = ZeroNumber,
              .y = ZeroNumber,
              .z = UntypedNumericValue::number(radians)};
        }

        if constexpr (std::is_same_v<T, CSSRotateX>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = UntypedNumericValue::number(radians),
              .y = ZeroNumber,
              .z = ZeroNumber};
        }

        if constexpr (std::is_same_v<T, CSSRotateY>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = ZeroNumber,
              .y = UntypedNumericValue::number(radians),
              .z = ZeroNumber};
        }

        if constexpr (std::is_same_v<T, CSSRotateZ>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = ZeroNumber,
              .y = ZeroNumber,
              .z = UntypedNumericValue::number(radians)};
        }

        if constexpr (std::is_same_v<T, CSSTranslate>) {
          auto x = numericValueFromCSSLengthPercentage(func.x);
          auto y = numericValueFromCSSLengthPercentage(func.y);
          if (!x || !y) {
            return std::nullopt;
          }
          return TransformOperation{
              .type = TransformOperationType::Translate,
              .x = x,
              .y = y,
              .z = ZeroLength};
        }

        if constexpr (std::is_same_v<T, CSSTranslateX>) {
          auto x = numericValueFromCSSLengthPercentage(func.value);
          if (!x) {
            return std::nullopt;
          }
          return TransformOperation{
              .type = TransformOperationType::Translate,
              .x = x,
              .y = ZeroLength,
              .z = ZeroLength};
        }

        if constexpr (std::is_same_v<T, CSSTranslateY>) {
          auto y = numericValueFromCSSLengthPercentage(func.value);
          if (!y) {
            return std::nullopt;
          }
          return TransformOperation{
              .type = TransformOperationType::Translate,
              .x = ZeroLength,
              .y = y,
              .z = ZeroLength};
        }

        if constexpr (std::is_same_v<T, CSSTranslate3D>) {
          auto x = numericValueFromCSSLengthPercentage(func.x);
          auto y = numericValueFromCSSLengthPercentage(func.y);
          if (!x || !y || func.z.unit != CSSLengthUnit::Px) {
            return std::nullopt;
          }
          return TransformOperation{
              .type = TransformOperationType::Translate,
              .x = x,
              .y = y,
              .z = UntypedNumericValue::length(func.z.value)};
        }

        if constexpr (std::is_same_v<T, CSSScale>) {
          return TransformOperation{
              .type = TransformOperationType::Scale,
              .x = UntypedNumericValue::number(func.x),
              .y = UntypedNumericValue::number(func.y),
              .z = One};
        }

        if constexpr (std::is_same_v<T, CSSScaleX>) {
          return TransformOperation{
              .type = TransformOperationType::Scale,
              .x = UntypedNumericValue::number(func.value),
              .y = One,
              .z = One};
        }

        if constexpr (std::is_same_v<T, CSSScaleY>) {
          return TransformOperation{
              .type = TransformOperationType::Scale,
              .x = One,
              .y = UntypedNumericValue::number(func.value),
              .z = One};
        }

        if constexpr (std::is_same_v<T, CSSSkewX>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Skew,
              .x = UntypedNumericValue::number(radians),
              .y = ZeroNumber,
              .z = ZeroNumber};
        }

        if constexpr (std::is_same_v<T, CSSSkewY>) {
          auto radians = static_cast<float>(func.degrees * M_PI / 180.0f);
          return TransformOperation{
              .type = TransformOperationType::Skew,
              .x = ZeroNumber,
              .y = UntypedNumericValue::number(radians),
              .z = ZeroNumber};
        }

        if constexpr (std::is_same_v<T, CSSPerspective>) {
          if (func.length.unit != CSSLengthUnit::Px) {
            return std::nullopt;
          }
          return TransformOperation{
              .type = TransformOperationType::Perspective,
              .x = UntypedNumericValue::length(func.length.value),
              .y = ZeroNumber,
              .z = ZeroNumber};
        }

        if constexpr (std::is_same_v<T, CSSMatrix>) {
          return TransformOperation{
              .type = TransformOperationType::Arbitrary,
              .x = ZeroNumber,
              .y = ZeroNumber,
              .z = ZeroNumber};
        }
      },
      cssTransform);
}

inline void parseProcessedTransform(
    const PropsParserContext& context,
    const RawValue& value,
    Transform& result) {
  auto transformMatrix = Transform{};
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = transformMatrix;
    return;
  }

  auto configurations = static_cast<std::vector<RawValue>>(value);
  for (const auto& configuration : configurations) {
    if (!configuration.hasType<std::unordered_map<std::string, RawValue>>()) {
      result = {};
      return;
    }

    auto configurationPair =
        static_cast<std::unordered_map<std::string, RawValue>>(configuration);
    if (configurationPair.size() != 1) {
      result = {};
      return;
    }

    auto pair = configurationPair.begin();
    auto operation = pair->first;
    auto& parameters = pair->second;
    auto ZeroNumber = UntypedNumericValue::number(0.0f);
    auto ZeroLength = UntypedNumericValue::length(0.0f);
    auto One = UntypedNumericValue::number(1.0f);

    if (operation == "matrix") {
      // T215634510: We should support matrix transforms as part of a list of
      // transforms
      if (configurations.size() > 1) {
        result = {};
        return;
      }

      if (!parameters.hasType<std::vector<Float>>()) {
        result = {};
        return;
      }

      auto numbers = (std::vector<Float>)parameters;
      if (numbers.size() != 9 && numbers.size() != 16) {
        result = {};
        return;
      }

      if (numbers.size() == 16) {
        size_t i = 0;

        for (auto number : numbers) {
          transformMatrix.matrix[i++] = number;
        }
      } else if (numbers.size() == 9) {
        // We need to convert the 2d transform matrix into a 3d one as such:
        // [
        //   x00, x01, 0, x02
        //   x10, x11, 0, x12
        //   0,   0,   1, 0
        //   x20, x21, 0, x22
        // ]
        transformMatrix.matrix[0] = numbers[0];
        transformMatrix.matrix[1] = numbers[1];
        transformMatrix.matrix[2] = 0;
        transformMatrix.matrix[3] = numbers[2];
        transformMatrix.matrix[4] = numbers[3];
        transformMatrix.matrix[5] = numbers[4];
        transformMatrix.matrix[6] = 0;
        transformMatrix.matrix[7] = numbers[5];
        transformMatrix.matrix[8] = 0;
        transformMatrix.matrix[9] = 0;
        transformMatrix.matrix[10] = 1;
        transformMatrix.matrix[11] = 0;
        transformMatrix.matrix[12] = numbers[6];
        transformMatrix.matrix[13] = numbers[7];
        transformMatrix.matrix[14] = 0;
        transformMatrix.matrix[15] = numbers[8];
      }
      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Arbitrary,
              .x = ZeroNumber,
              .y = ZeroNumber,
              .z = ZeroNumber});
    } else if (operation == "perspective") {
      auto perspective =
          parseNumericValueAs<NumericValueLength>(context, parameters);
      if (!perspective) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Perspective,
              .x = *perspective,
              .y = ZeroNumber,
              .z = ZeroNumber});
    } else if (operation == "rotateX") {
      auto radians = toRadians(parameters);
      if (!radians.has_value()) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = UntypedNumericValue::number(*radians),
              .y = ZeroNumber,
              .z = ZeroNumber});
    } else if (operation == "rotateY") {
      auto radians = toRadians(parameters);
      if (!radians.has_value()) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = ZeroNumber,
              .y = UntypedNumericValue::number(*radians),
              .z = ZeroNumber});
    } else if (operation == "rotateZ" || operation == "rotate") {
      auto radians = toRadians(parameters);
      if (!radians.has_value()) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Rotate,
              .x = ZeroNumber,
              .y = ZeroNumber,
              .z = UntypedNumericValue::number(*radians)});
    } else if (operation == "scale") {
      auto scale = parseScaleNumericValue(context, parameters);
      if (!scale) {
        result = {};
        return;
      }
      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Scale,
              .x = *scale,
              .y = *scale,
              .z = *scale});
    } else if (operation == "scaleX") {
      auto scaleX = parseScaleNumericValue(context, parameters);
      if (!scaleX) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Scale,
              .x = *scaleX,
              .y = One,
              .z = One});
    } else if (operation == "scaleY") {
      auto scaleY = parseScaleNumericValue(context, parameters);
      if (!scaleY) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Scale,
              .x = One,
              .y = *scaleY,
              .z = One});
    } else if (operation == "scaleZ") {
      auto scaleZ = parseScaleNumericValue(context, parameters);
      if (!scaleZ) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Scale,
              .x = One,
              .y = One,
              .z = *scaleZ});
    } else if (operation == "translate") {
      if (!parameters.hasType<std::vector<RawValue>>()) {
        result = {};
        return;
      }

      auto numbers = (std::vector<RawValue>)parameters;
      if (numbers.size() != 2) {
        result = {};
        return;
      }

      auto valueX = parseNumericValueAs<NumericValueLengthPercentage>(
          context, numbers[0]);
      if (!valueX) {
        result = {};
        return;
      }

      auto valueY = parseNumericValueAs<NumericValueLengthPercentage>(
          context, numbers[1]);
      if (!valueY) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Translate,
              .x = *valueX,
              .y = *valueY,
              .z = ZeroLength});
    } else if (operation == "translateX") {
      auto valueX = parseNumericValueAs<NumericValueLengthPercentage>(
          context, parameters);
      if (!valueX) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Translate,
              .x = *valueX,
              .y = ZeroLength,
              .z = ZeroLength});
    } else if (operation == "translateY") {
      auto valueY = parseNumericValueAs<NumericValueLengthPercentage>(
          context, parameters);
      if (!valueY) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Translate,
              .x = ZeroLength,
              .y = *valueY,
              .z = ZeroLength});
    } else if (operation == "skewX") {
      auto radians = toRadians(parameters);
      if (!radians.has_value()) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Skew,
              .x = UntypedNumericValue::number(*radians),
              .y = ZeroNumber,
              .z = ZeroNumber});
    } else if (operation == "skewY") {
      auto radians = toRadians(parameters);
      if (!radians.has_value()) {
        result = {};
        return;
      }

      transformMatrix.operations.push_back(
          TransformOperation{
              .type = TransformOperationType::Skew,
              .x = ZeroNumber,
              .y = UntypedNumericValue::number(*radians),
              .z = ZeroNumber});
    }
  }

  result = transformMatrix;
}

inline void parseUnprocessedTransformString(
    const std::string& value,
    Transform& result) {
  auto transformList = parseCSSProperty<CSSTransformList>(value);
  if (!std::holds_alternative<CSSTransformList>(transformList)) {
    result = {};
    return;
  }

  auto transformMatrix = Transform{};
  const auto& cssFuncs = std::get<CSSTransformList>(transformList);
  transformMatrix.operations.reserve(cssFuncs.size());
  for (const auto& cssFunc : cssFuncs) {
    auto op = fromCSSTransformFunction(cssFunc);
    if (!op.has_value()) {
      result = {};
      return;
    }

    if (op->type == TransformOperationType::Arbitrary) {
      // CSSMatrix: expand 6-value 2D matrix to 4x4 matrix
      if (std::holds_alternative<CSSMatrix>(cssFunc)) {
        const auto& m = std::get<CSSMatrix>(cssFunc);
        transformMatrix.matrix[0] = m.values[0];
        transformMatrix.matrix[1] = m.values[1];
        transformMatrix.matrix[2] = 0;
        transformMatrix.matrix[3] = 0;
        transformMatrix.matrix[4] = m.values[2];
        transformMatrix.matrix[5] = m.values[3];
        transformMatrix.matrix[6] = 0;
        transformMatrix.matrix[7] = 0;
        transformMatrix.matrix[8] = 0;
        transformMatrix.matrix[9] = 0;
        transformMatrix.matrix[10] = 1;
        transformMatrix.matrix[11] = 0;
        transformMatrix.matrix[12] = m.values[4];
        transformMatrix.matrix[13] = m.values[5];
        transformMatrix.matrix[14] = 0;
        transformMatrix.matrix[15] = 1;
      }
    }

    transformMatrix.operations.push_back(*op);
  }

  result = transformMatrix;
}

inline void parseUnprocessedTransform(
    const PropsParserContext& context,
    const RawValue& value,
    Transform& result) {
  if (value.hasType<std::string>()) {
    parseUnprocessedTransformString((std::string)value, result);
  } else {
    parseProcessedTransform(context, value, result);
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    Transform& result) {
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    parseUnprocessedTransform(context, value, result);
  } else {
    parseProcessedTransform(context, value, result);
  }
}

inline void parseProcessedTransformOrigin(
    const PropsParserContext& context,
    const RawValue& value,
    TransformOrigin& result) {
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  auto origins = (std::vector<RawValue>)value;
  if (origins.size() != 3) {
    result = {};
    return;
  }

  TransformOrigin transformOrigin;

  for (size_t i = 0; i < 2; i++) {
    auto origin =
        parseNumericValue<NumericValueLengthPercentage>(context, origins[i]);
    if (!origin) {
      result = {};
      return;
    }

    transformOrigin.xy[i] = *origin;
  }

  if (!origins[2].hasType<Float>()) {
    result = {};
    return;
  }
  transformOrigin.z = (Float)origins[2];

  result = transformOrigin;
}

inline void parseUnprocessedTransformOriginString(
    const std::string& value,
    TransformOrigin& result) {
  auto cssOrigin = parseCSSProperty<CSSTransformOrigin>(value);
  if (!std::holds_alternative<CSSTransformOrigin>(cssOrigin)) {
    result = {};
    return;
  }

  const auto& origin = std::get<CSSTransformOrigin>(cssOrigin);
  TransformOrigin transformOrigin;

  auto x = numericValueFromCSSLengthPercentage(origin.x);
  auto y = numericValueFromCSSLengthPercentage(origin.y);
  if (!x || !y) {
    result = {};
    return;
  }

  transformOrigin.xy[0] = x;
  transformOrigin.xy[1] = y;

  if (origin.z.unit != CSSLengthUnit::Px) {
    result = {};
    return;
  }
  transformOrigin.z = origin.z.value;

  result = transformOrigin;
}

inline void parseUnprocessedTransformOrigin(
    const PropsParserContext& context,
    const RawValue& value,
    TransformOrigin& result) {
  if (value.hasType<std::string>()) {
    parseUnprocessedTransformOriginString((std::string)value, result);
  } else {
    parseProcessedTransformOrigin(context, value, result);
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    TransformOrigin& result) {
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    parseUnprocessedTransformOrigin(context, value, result);
  } else {
    parseProcessedTransformOrigin(context, value, result);
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    PointerEventsMode& result) {
  result = PointerEventsMode::Auto;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "auto") {
    result = PointerEventsMode::Auto;
    return;
  }
  if (stringValue == "none") {
    result = PointerEventsMode::None;
    return;
  }
  if (stringValue == "box-none") {
    result = PointerEventsMode::BoxNone;
    return;
  }
  if (stringValue == "box-only") {
    result = PointerEventsMode::BoxOnly;
    return;
  }
  LOG(ERROR) << "Could not parse PointerEventsMode:" << stringValue;
  react_native_expect(false);
}

inline std::string toString(PointerEventsMode value) {
  switch (value) {
    case PointerEventsMode::Auto:
      return "auto";
    case PointerEventsMode::None:
      return "none";
    case PointerEventsMode::BoxNone:
      return "box-none";
    case PointerEventsMode::BoxOnly:
      return "box-only";
  }
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    BackfaceVisibility& result) {
  result = BackfaceVisibility::Auto;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "auto") {
    result = BackfaceVisibility::Auto;
    return;
  }
  if (stringValue == "visible") {
    result = BackfaceVisibility::Visible;
    return;
  }
  if (stringValue == "hidden") {
    result = BackfaceVisibility::Hidden;
    return;
  }
  LOG(ERROR) << "Could not parse BackfaceVisibility:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    BorderCurve& result) {
  result = BorderCurve::Circular;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "circular") {
    result = BorderCurve::Circular;
    return;
  }
  if (stringValue == "continuous") {
    result = BorderCurve::Continuous;
    return;
  }
  LOG(ERROR) << "Could not parse BorderCurve:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    BorderStyle& result) {
  result = BorderStyle::Solid;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "solid") {
    result = BorderStyle::Solid;
    return;
  }
  if (stringValue == "dotted") {
    result = BorderStyle::Dotted;
    return;
  }
  if (stringValue == "dashed") {
    result = BorderStyle::Dashed;
    return;
  }
  LOG(ERROR) << "Could not parse BorderStyle:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    OutlineStyle& result) {
  result = OutlineStyle::Solid;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "solid") {
    result = OutlineStyle::Solid;
    return;
  }
  if (stringValue == "dotted") {
    result = OutlineStyle::Dotted;
    return;
  }
  if (stringValue == "dashed") {
    result = OutlineStyle::Dashed;
    return;
  }
  LOG(ERROR) << "Could not parse OutlineStyle:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    Cursor& result) {
  result = Cursor::Auto;
  react_native_expect(value.hasType<std::string>());
  if (!value.hasType<std::string>()) {
    return;
  }
  auto stringValue = (std::string)value;
  if (stringValue == "alias") {
    result = Cursor::Alias;
    return;
  }
  if (stringValue == "all-scroll") {
    result = Cursor::AllScroll;
    return;
  }
  if (stringValue == "auto") {
    result = Cursor::Auto;
    return;
  }
  if (stringValue == "cell") {
    result = Cursor::Cell;
    return;
  }
  if (stringValue == "col-resize") {
    result = Cursor::ColResize;
    return;
  }
  if (stringValue == "context-menu") {
    result = Cursor::ContextMenu;
    return;
  }
  if (stringValue == "copy") {
    result = Cursor::Copy;
    return;
  }
  if (stringValue == "crosshair") {
    result = Cursor::Crosshair;
    return;
  }
  if (stringValue == "default") {
    result = Cursor::Default;
    return;
  }
  if (stringValue == "e-resize") {
    result = Cursor::EResize;
    return;
  }
  if (stringValue == "ew-resize") {
    result = Cursor::EWResize;
    return;
  }
  if (stringValue == "grab") {
    result = Cursor::Grab;
    return;
  }
  if (stringValue == "grabbing") {
    result = Cursor::Grabbing;
    return;
  }
  if (stringValue == "help") {
    result = Cursor::Help;
    return;
  }
  if (stringValue == "move") {
    result = Cursor::Move;
    return;
  }
  if (stringValue == "n-resize") {
    result = Cursor::NResize;
    return;
  }
  if (stringValue == "ne-resize") {
    result = Cursor::NEResize;
    return;
  }
  if (stringValue == "nesw-resize") {
    result = Cursor::NESWResize;
    return;
  }
  if (stringValue == "ns-resize") {
    result = Cursor::NSResize;
    return;
  }
  if (stringValue == "nw-resize") {
    result = Cursor::NWResize;
    return;
  }
  if (stringValue == "nwse-resize") {
    result = Cursor::NWSEResize;
    return;
  }
  if (stringValue == "no-drop") {
    result = Cursor::NoDrop;
    return;
  }
  if (stringValue == "none") {
    result = Cursor::None;
    return;
  }
  if (stringValue == "not-allowed") {
    result = Cursor::NotAllowed;
    return;
  }
  if (stringValue == "pointer") {
    result = Cursor::Pointer;
    return;
  }
  if (stringValue == "progress") {
    result = Cursor::Progress;
    return;
  }
  if (stringValue == "row-resize") {
    result = Cursor::RowResize;
    return;
  }
  if (stringValue == "s-resize") {
    result = Cursor::SResize;
    return;
  }
  if (stringValue == "se-resize") {
    result = Cursor::SEResize;
    return;
  }
  if (stringValue == "sw-resize") {
    result = Cursor::SWResize;
    return;
  }
  if (stringValue == "text") {
    result = Cursor::Text;
    return;
  }
  if (stringValue == "url") {
    result = Cursor::Url;
    return;
  }
  if (stringValue == "w-resize") {
    result = Cursor::WResize;
    return;
  }
  if (stringValue == "wait") {
    result = Cursor::Wait;
    return;
  }
  if (stringValue == "zoom-in") {
    result = Cursor::ZoomIn;
    return;
  }
  if (stringValue == "zoom-out") {
    result = Cursor::ZoomOut;
    return;
  }
  LOG(ERROR) << "Could not parse Cursor:" << stringValue;
  react_native_expect(false);
}

inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    LayoutConformance& result) {
  react_native_expect(value.hasType<std::string>());
  result = LayoutConformance::Strict;
  if (!value.hasType<std::string>()) {
    return;
  }

  auto stringValue = (std::string)value;
  if (stringValue == "strict") {
    result = LayoutConformance::Strict;
  } else if (stringValue == "compatibility") {
    result = LayoutConformance::Compatibility;
  } else {
    LOG(ERROR) << "Unexpected LayoutConformance value:" << stringValue;
    react_native_expect(false);
  }
}

inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    BlendMode& result) {
  react_native_expect(value.hasType<std::string>());
  result = BlendMode::Normal;
  if (!value.hasType<std::string>()) {
    return;
  }

  auto rawBlendMode = static_cast<std::string>(value);
  std::optional<BlendMode> blendMode = blendModeFromString(rawBlendMode);

  if (!blendMode) {
    LOG(ERROR) << "Could not parse blend mode: " << rawBlendMode;
    return;
  }

  result = blendMode.value();
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BackgroundSize>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  std::vector<BackgroundSize> backgroundSizes{};
  auto rawBackgroundSizes = static_cast<std::vector<RawValue>>(value);

  for (const auto& rawBackgroundSizeValue : rawBackgroundSizes) {
    if (rawBackgroundSizeValue.hasType<std::string>()) {
      auto sizeStr = (std::string)rawBackgroundSizeValue;
      if (sizeStr == "cover") {
        backgroundSizes.emplace_back(BackgroundSizeKeyword::Cover);
      } else if (sizeStr == "contain") {
        backgroundSizes.emplace_back(BackgroundSizeKeyword::Contain);
      }
    } else if (
        rawBackgroundSizeValue
            .hasType<std::unordered_map<std::string, RawValue>>()) {
      auto sizeMap = static_cast<std::unordered_map<std::string, RawValue>>(
          rawBackgroundSizeValue);

      BackgroundSizeLengthPercentage sizeLengthPercentage;

      auto xIt = sizeMap.find("x");
      if (xIt != sizeMap.end()) {
        if (xIt->second.hasType<std::string>() &&
            (std::string)(xIt->second) == "auto") {
          sizeLengthPercentage.x = std::monostate{};
        } else {
          auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
              context, xIt->second);
          if (numericValue) {
            sizeLengthPercentage.x = *numericValue;
          }
        }
      }

      auto yIt = sizeMap.find("y");
      if (yIt != sizeMap.end()) {
        if (yIt->second.hasType<std::string>() &&
            (std::string)(yIt->second) == "auto") {
          sizeLengthPercentage.y = std::monostate{};
        } else {
          auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
              context, yIt->second);
          if (numericValue) {
            sizeLengthPercentage.y = *numericValue;
          }
        }
      }

      backgroundSizes.emplace_back(sizeLengthPercentage);
    }
  }

  result = backgroundSizes;
}

inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    std::vector<BackgroundPosition>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  std::vector<BackgroundPosition> backgroundPositions{};
  auto rawBackgroundPositions = static_cast<std::vector<RawValue>>(value);

  for (const auto& rawBackgroundPositionValue : rawBackgroundPositions) {
    if (rawBackgroundPositionValue
            .hasType<std::unordered_map<std::string, RawValue>>()) {
      auto positionMap = static_cast<std::unordered_map<std::string, RawValue>>(
          rawBackgroundPositionValue);

      BackgroundPosition backgroundPosition;

      auto topIt = positionMap.find("top");
      if (topIt != positionMap.end()) {
        auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
            context, topIt->second);
        if (numericValue) {
          backgroundPosition.top = *numericValue;
        }
      }

      auto bottomIt = positionMap.find("bottom");
      if (bottomIt != positionMap.end()) {
        auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
            context, bottomIt->second);
        if (numericValue) {
          backgroundPosition.bottom = *numericValue;
        }
      }

      auto leftIt = positionMap.find("left");
      if (leftIt != positionMap.end()) {
        auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
            context, leftIt->second);
        if (numericValue) {
          backgroundPosition.left = *numericValue;
        }
      }

      auto rightIt = positionMap.find("right");
      if (rightIt != positionMap.end()) {
        auto numericValue = parseNumericValue<NumericValueLengthPercentage>(
            context, rightIt->second);
        if (numericValue) {
          backgroundPosition.right = *numericValue;
        }
      }

      backgroundPositions.emplace_back(backgroundPosition);
    }
  }

  result = backgroundPositions;
}

inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    std::vector<BackgroundRepeat>& result) {
  react_native_expect(value.hasType<std::vector<RawValue>>());
  if (!value.hasType<std::vector<RawValue>>()) {
    result = {};
    return;
  }

  std::vector<BackgroundRepeat> backgroundRepeats{};
  auto rawBackgroundRepeats = static_cast<std::vector<RawValue>>(value);

  for (const auto& rawBackgroundRepeatValue : rawBackgroundRepeats) {
    if (rawBackgroundRepeatValue
            .hasType<std::unordered_map<std::string, RawValue>>()) {
      auto repeatMap = static_cast<std::unordered_map<std::string, RawValue>>(
          rawBackgroundRepeatValue);

      BackgroundRepeat backgroundRepeat;

      auto xIt = repeatMap.find("x");
      if (xIt != repeatMap.end() && xIt->second.hasType<std::string>()) {
        auto xStr = (std::string)(xIt->second);
        if (xStr == "repeat") {
          backgroundRepeat.x = BackgroundRepeatStyle::Repeat;
        } else if (xStr == "space") {
          backgroundRepeat.x = BackgroundRepeatStyle::Space;
        } else if (xStr == "round") {
          backgroundRepeat.x = BackgroundRepeatStyle::Round;
        } else if (xStr == "no-repeat") {
          backgroundRepeat.x = BackgroundRepeatStyle::NoRepeat;
        }
      }

      auto yIt = repeatMap.find("y");
      if (yIt != repeatMap.end() && yIt->second.hasType<std::string>()) {
        auto yStr = (std::string)(yIt->second);
        if (yStr == "repeat") {
          backgroundRepeat.y = BackgroundRepeatStyle::Repeat;
        } else if (yStr == "space") {
          backgroundRepeat.y = BackgroundRepeatStyle::Space;
        } else if (yStr == "round") {
          backgroundRepeat.y = BackgroundRepeatStyle::Round;
        } else if (yStr == "no-repeat") {
          backgroundRepeat.y = BackgroundRepeatStyle::NoRepeat;
        }
      }

      backgroundRepeats.emplace_back(backgroundRepeat);
    }
  }

  result = backgroundRepeats;
}

inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    Isolation& result) {
  react_native_expect(value.hasType<std::string>());
  result = Isolation::Auto;
  if (!value.hasType<std::string>()) {
    return;
  }

  auto rawIsolation = static_cast<std::string>(value);
  std::optional<Isolation> isolation = isolationFromString(rawIsolation);

  if (!isolation) {
    LOG(ERROR) << "Could not parse isolation: " << rawIsolation;
    return;
  }

  result = isolation.value();
}

#if RN_DEBUG_STRING_CONVERTIBLE
template <size_t N>
inline std::string toString(const std::array<float, N> vec) {
  std::string s;

  s.append("{");
  for (size_t i = 0; i < N - 1; i++) {
    s.append(toString(vec[i]) + ", ");
  }
  s.append(toString(vec[N - 1]));
  s.append("}");

  return s;
}

inline std::string toString(const yoga::Direction& value) {
  return YGDirectionToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::FlexDirection& value) {
  return YGFlexDirectionToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Justify& value) {
  return YGJustifyToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Align& value) {
  return YGAlignToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::PositionType& value) {
  return YGPositionTypeToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Wrap& value) {
  return YGWrapToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Overflow& value) {
  return YGOverflowToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Display& value) {
  return YGDisplayToString(yoga::unscopedEnum(value));
}

inline std::string toString(const yoga::Style::Length& length) {
  if (length.isUndefined()) {
    return "undefined";
  } else if (length.isAuto()) {
    return "auto";
  } else if (length.isPoints()) {
    return toString(length.value().unwrap());
  } else if (length.isPercent()) {
    return toString(length.value().unwrap()) + "%";
  } else {
    return "unknown";
  }
}

inline std::string toString(const yoga::Style::SizeLength& length) {
  if (length.isUndefined()) {
    return "undefined";
  } else if (length.isAuto()) {
    return "auto";
  } else if (length.isPoints()) {
    return toString(length.value().unwrap());
  } else if (length.isPercent()) {
    return toString(length.value().unwrap()) + "%";
  } else if (length.isMaxContent()) {
    return "max-content";
  } else if (length.isFitContent()) {
    return "fit-content";
  } else if (length.isStretch()) {
    return "stretch";
  } else {
    return "unknown";
  }
}

inline std::string toString(const yoga::FloatOptional& value) {
  if (value.isUndefined()) {
    return "undefined";
  }

  return toString(value.unwrap());
}

inline std::string toString(const LayoutConformance& value) {
  switch (value) {
    case LayoutConformance::Strict:
      return "strict";
    case LayoutConformance::Compatibility:
      return "compatibility";
  }
}

inline std::string toString(const std::array<Float, 16>& m) {
  std::string result;
  result += "[ " + toString(m[0]) + " " + toString(m[1]) + " " +
      toString(m[2]) + " " + toString(m[3]) + " ]\n";
  result += "[ " + toString(m[4]) + " " + toString(m[5]) + " " +
      toString(m[6]) + " " + toString(m[7]) + " ]\n";
  result += "[ " + toString(m[8]) + " " + toString(m[9]) + " " +
      toString(m[10]) + " " + toString(m[11]) + " ]\n";
  result += "[ " + toString(m[12]) + " " + toString(m[13]) + " " +
      toString(m[14]) + " " + toString(m[15]) + " ]";
  return result;
}

inline std::string toString(const Transform& transform) {
  std::string result = "[";
  bool first = true;

  for (const auto& operation : transform.operations) {
    if (!first) {
      result += ", ";
    }
    first = false;

    switch (operation.type) {
      case TransformOperationType::Perspective: {
        result += "{\"perspective\": " + toString(operation.x.asFloat()) + "}";
        break;
      }
      case TransformOperationType::Rotate: {
        if (operation.x.asFloat() != 0 && operation.y.asFloat() == 0 &&
            operation.z.asFloat() == 0) {
          result +=
              R"({"rotateX": ")" + toString(operation.x.asFloat()) + "rad\"}";
        } else if (
            operation.x.asFloat() == 0 && operation.y.asFloat() != 0 &&
            operation.z.asFloat() == 0) {
          result +=
              R"({"rotateY": ")" + toString(operation.y.asFloat()) + "rad\"}";
        } else if (
            operation.x.asFloat() == 0 && operation.y.asFloat() == 0 &&
            operation.z.asFloat() != 0) {
          result +=
              R"({"rotateZ": ")" + toString(operation.z.asFloat()) + "rad\"}";
        }
        break;
      }
      case TransformOperationType::Scale: {
        if (operation.x.asFloat() == operation.y.asFloat() &&
            operation.x.asFloat() == operation.z.asFloat()) {
          result += "{\"scale\": " + toString(operation.x.asFloat()) + "}";
        } else if (operation.y.asFloat() == 1 && operation.z.asFloat() == 1) {
          result += "{\"scaleX\": " + toString(operation.x.asFloat()) + "}";
        } else if (operation.x.asFloat() == 1 && operation.z.asFloat() == 1) {
          result += "{\"scaleY\": " + toString(operation.y.asFloat()) + "}";
        } else if (operation.x.asFloat() == 1 && operation.y.asFloat() == 1) {
          result += "{\"scaleZ\": " + toString(operation.z.asFloat()) + "}";
        }
        break;
      }
      case TransformOperationType::Translate: {
        if (operation.x.asFloat() != 0 && operation.y.asFloat() != 0 &&
            operation.z.asFloat() == 0) {
          result += "{\"translate\": [";
          result += toString(operation.x.asFloat()) + ", " +
              toString(operation.y.asFloat());
          result += "]}";
        } else if (operation.x.asFloat() != 0 && operation.y.asFloat() == 0) {
          result += "{\"translateX\": " + toString(operation.x.asFloat()) + "}";
        } else if (operation.x.asFloat() == 0 && operation.y.asFloat() != 0) {
          result += "{\"translateY\": " + toString(operation.y.asFloat()) + "}";
        }
        break;
      }
      case TransformOperationType::Skew: {
        if (operation.x.asFloat() != 0 && operation.y.asFloat() == 0) {
          result +=
              R"({"skewX": ")" + toString(operation.x.asFloat()) + "rad\"}";
        } else if (operation.x.asFloat() == 0 && operation.y.asFloat() != 0) {
          result +=
              R"({"skewY": ")" + toString(operation.y.asFloat()) + "rad\"}";
        }
        break;
      }
      case TransformOperationType::Arbitrary: {
        result += "{\"matrix\": " + toString(transform.matrix) + "}";
        break;
      }
      case TransformOperationType::Identity: {
        result += "{\"identity\": true}";
        break;
      }
    }
  }

  result += "]";
  return result;
}
#endif

} // namespace facebook::react
