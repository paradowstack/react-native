/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "BaseViewProps.h"

#include <folly/json.h>
#include <algorithm>
#include <cmath>

#include <glog/logging.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/components/view/BackgroundImagePropsConversions.h>
#include <react/renderer/components/view/BoxShadowPropsConversions.h>
#include <react/renderer/components/view/DynamicResolveContext.h>
#include <react/renderer/components/view/FilterPropsConversions.h>
#include <react/renderer/components/view/conversions.h>
#include <react/renderer/components/view/primitives.h>
#include <react/renderer/components/view/propsConversions.h>
#include <react/renderer/core/graphicsConversions.h>
#include <react/renderer/core/propsConversions.h>
#include <react/renderer/debug/debugStringConvertibleUtils.h>
#include <react/renderer/graphics/NumericValue.h>
#include <react/renderer/graphics/ValueUnit.h>

namespace facebook::react {

namespace {

// Computes the CSS gradient-line length for a linear gradient:
// L = |W * sin(θ)| + |H * cos(θ)|
// where θ is the gradient angle in degrees (0 = to top, 90 = to right).
// For GradientKeyword directions the equivalent angle depends on frame size.
static float linearGradientLineLength(
    const GradientDirection& direction,
    float width,
    float height) {
  float angleDeg = 0.0f;
  if (std::holds_alternative<Float>(direction)) {
    angleDeg = std::get<Float>(direction);
  } else {
    switch (std::get<GradientKeyword>(direction)) {
      case GradientKeyword::ToTopRight:
        angleDeg = 90.0f - std::atan2(width, height) * 180.0f / M_PI;
        break;
      case GradientKeyword::ToBottomRight:
        angleDeg = std::atan2(width, height) * 180.0f / M_PI + 90.0f;
        break;
      case GradientKeyword::ToTopLeft:
        angleDeg = std::atan2(width, height) * 180.0f / M_PI + 270.0f;
        break;
      case GradientKeyword::ToBottomLeft:
        angleDeg = std::atan2(height, width) * 180.0f / M_PI + 180.0f;
        break;
    }
  }
  float rad = angleDeg * static_cast<float>(M_PI) / 180.0f;
  return std::abs(width * std::sin(rad)) + std::abs(height * std::cos(rad));
}

std::array<float, 3> getTranslateForTransformOrigin(
    float viewWidth,
    float viewHeight,
    TransformOrigin transformOrigin) {
  float viewCenterX = viewWidth / 2;
  float viewCenterY = viewHeight / 2;

  std::array<float, 3> origin = {viewCenterX, viewCenterY, transformOrigin.z};

  for (size_t i = 0; i < transformOrigin.xy.size(); ++i) {
    auto& currentOrigin = transformOrigin.xy[i];
    if (currentOrigin.isLength()) {
      origin[i] = currentOrigin.asFloat();
    } else if (currentOrigin.isPercentage()) {
      origin[i] = ((i == 0) ? viewWidth : viewHeight) *
          currentOrigin.asFloat() / 100.0f;
    }
  }

  float newTranslateX = -viewCenterX + origin[0];
  float newTranslateY = -viewCenterY + origin[1];
  float newTranslateZ = origin[2];

  return std::array{newTranslateX, newTranslateY, newTranslateZ};
}

} // namespace

BaseViewProps::BaseViewProps(
    const PropsParserContext& context,
    const BaseViewProps& sourceProps,
    const RawProps& rawProps,
    const std::function<bool(const std::string&)>& filterObjectKeys)
    : YogaStylableProps(context, sourceProps, rawProps, filterObjectKeys),
      AccessibilityProps(context, sourceProps, rawProps),
      opacity(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.opacity
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "opacity",
                    sourceProps.opacity,
                    1.0f)),
      backgroundColor(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backgroundColor
              : convertRawProp(
                    context,
                    rawProps,
                    "backgroundColor",
                    sourceProps.backgroundColor,
                    {})),
      borderRadii(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderRadii
              : convertRawProp(
                    context,
                    rawProps,
                    CascadedRectangleCornersNames{
                        .topLeft = "borderTopLeftRadius",
                        .topRight = "borderTopRightRadius",
                        .bottomLeft = "borderBottomLeftRadius",
                        .bottomRight = "borderBottomRightRadius",
                        .topStart = "borderTopStartRadius",
                        .topEnd = "borderTopEndRadius",
                        .bottomStart = "borderBottomStartRadius",
                        .bottomEnd = "borderBottomEndRadius",
                        .endEnd = "borderEndEndRadius",
                        .endStart = "borderEndStartRadius",
                        .startEnd = "borderStartEndRadius",
                        .startStart = "borderStartStartRadius",
                        .all = "borderRadius"},
                    sourceProps.borderRadii,
                    {})),
      borderColors(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderColors
              : convertRawProp(
                    context,
                    rawProps,
                    CascadedRectangleEdgesNames{
                        .left = "borderLeftColor",
                        .right = "borderRightColor",
                        .top = "borderTopColor",
                        .bottom = "borderBottomColor",
                        .start = "borderStartColor",
                        .end = "borderEndColor",
                        .horizontal = "borderHorizontalColor",
                        .vertical = "borderVerticalColor",
                        .block = "borderBlockColor",
                        .blockEnd = "borderBlockEndColor",
                        .blockStart = "borderBlockStartColor",
                        .all = "borderColor"},
                    sourceProps.borderColors,
                    {})),
      borderCurves(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderCurves
              : convertRawProp(
                    context,
                    rawProps,
                    CascadedRectangleCornersNames{
                        .topLeft = "borderTopLeftCurve",
                        .topRight = "borderTopRightCurve",
                        .bottomLeft = "borderBottomLeftCurve",
                        .bottomRight = "borderBottomRightCurve",
                        .topStart = "borderTopStartCurve",
                        .topEnd = "borderTopEndCurve",
                        .bottomStart = "borderBottomStartCurve",
                        .bottomEnd = "borderBottomEndCurve",
                        .endEnd = "borderEndEndCurve",
                        .endStart = "borderEndStartCurve",
                        .startEnd = "borderStartEndCurve",
                        .startStart = "borderStartStartCurve",
                        .all = "borderCurve"},
                    sourceProps.borderCurves,
                    {})),
      borderStyles(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderStyles
              : convertRawProp(
                    context,
                    rawProps,
                    CascadedRectangleEdgesNames{
                        .left = "borderLeftStyle",
                        .right = "borderRightStyle",
                        .top = "borderTopStyle",
                        .bottom = "borderBottomStyle",
                        .start = "borderStartStyle",
                        .end = "borderEndStyle",
                        .horizontal = "borderHorizontalStyle",
                        .vertical = "borderVerticalStyle",
                        .block = "borderBlockStyle",
                        .blockEnd = "borderBlockEndStyle",
                        .blockStart = "borderBlockStartStyle",
                        .all = "borderStyle"},
                    sourceProps.borderStyles,
                    {})),
      outlineColor(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.outlineColor
              : convertRawProp(
                    context,
                    rawProps,
                    "outlineColor",
                    sourceProps.outlineColor,
                    {})),
      outlineOffset(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.outlineOffset
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "outlineOffset",
                    sourceProps.outlineOffset,
                    0.0f)),
      outlineStyle(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.outlineStyle
              : convertRawProp(
                    context,
                    rawProps,
                    "outlineStyle",
                    sourceProps.outlineStyle,
                    {})),
      outlineWidth(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.outlineWidth
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "outlineWidth",
                    sourceProps.outlineWidth,
                    {})),
      shadowColor(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shadowColor
              : convertRawProp(
                    context,
                    rawProps,
                    "shadowColor",
                    sourceProps.shadowColor,
                    {})),
      shadowOffset(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shadowOffset
              : convertRawProp(
                    context,
                    rawProps,
                    "shadowOffset",
                    sourceProps.shadowOffset,
                    {})),
      shadowOpacity(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shadowOpacity
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "shadowOpacity",
                    sourceProps.shadowOpacity,
                    0.0f)),
      shadowRadius(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shadowRadius
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "shadowRadius",
                    sourceProps.shadowRadius,
                    3.0f)),
      cursor(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.cursor
              : convertRawProp(
                    context,
                    rawProps,
                    "cursor",
                    sourceProps.cursor,
                    {})),
      boxShadow(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.boxShadow
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "boxShadow",
                    sourceProps.boxShadow,
                    {})),
      filter(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.filter
              : convertRawProp(
                    context,
                    rawProps,
                    "filter",
                    sourceProps.filter,
                    {})),
      backgroundImage(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backgroundImage
              : convertRawPropWithCalc(
                    context,
                    rawProps,
                    "backgroundImage",
                    convertRawProp(
                        context,
                        rawProps,
                        "experimental_backgroundImage",
                        sourceProps.backgroundImage,
                        {}),
                    {})),
      backgroundSize(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backgroundSize
              : convertRawProp(
                    context,
                    rawProps,
                    "experimental_backgroundSize",
                    sourceProps.backgroundSize,
                    {})),
      backgroundPosition(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backgroundPosition
              : convertRawProp(
                    context,
                    rawProps,
                    "experimental_backgroundPosition",
                    sourceProps.backgroundPosition,
                    {})),
      backgroundRepeat(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backgroundRepeat
              : convertRawProp(
                    context,
                    rawProps,
                    "experimental_backgroundRepeat",
                    sourceProps.backgroundRepeat,
                    {})),
      mixBlendMode(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.mixBlendMode
              : convertRawProp(
                    context,
                    rawProps,
                    "mixBlendMode",
                    sourceProps.mixBlendMode,
                    {})),
      isolation(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.isolation
              : convertRawProp(
                    context,
                    rawProps,
                    "isolation",
                    sourceProps.isolation,
                    {})),
      transform(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.transform
              : convertRawProp(
                    context,
                    rawProps,
                    "transform",
                    sourceProps.transform,
                    {})),
      transformOrigin(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.transformOrigin
              : convertRawProp(
                    context,
                    rawProps,
                    "transformOrigin",
                    sourceProps.transformOrigin,
                    {})),
      backfaceVisibility(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.backfaceVisibility
              : convertRawProp(
                    context,
                    rawProps,
                    "backfaceVisibility",
                    sourceProps.backfaceVisibility,
                    {})),
      shouldRasterize(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shouldRasterize
              : convertRawProp(
                    context,
                    rawProps,
                    "shouldRasterizeIOS",
                    sourceProps.shouldRasterize,
                    {})),
      zIndex(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.zIndex
              : convertRawProp(
                    context,
                    rawProps,
                    "zIndex",
                    sourceProps.zIndex,
                    {})),
      pointerEvents(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.pointerEvents
              : convertRawProp(
                    context,
                    rawProps,
                    "pointerEvents",
                    sourceProps.pointerEvents,
                    {})),
      hitSlop(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.hitSlop
              : convertRawProp(
                    context,
                    rawProps,
                    "hitSlop",
                    sourceProps.hitSlop,
                    {})),
      onLayout(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.onLayout
              : convertRawProp(
                    context,
                    rawProps,
                    "onLayout",
                    sourceProps.onLayout,
                    {})),
      events(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.events
              : convertRawProp(context, rawProps, sourceProps.events, {})),
      collapsable(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.collapsable
              : convertRawProp(
                    context,
                    rawProps,
                    "collapsable",
                    sourceProps.collapsable,
                    true)),
      collapsableChildren(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.collapsableChildren
              : convertRawProp(
                    context,
                    rawProps,
                    "collapsableChildren",
                    sourceProps.collapsableChildren,
                    true)),
      removeClippedSubviews(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.removeClippedSubviews
              : convertRawProp(
                    context,
                    rawProps,
                    "removeClippedSubviews",
                    sourceProps.removeClippedSubviews,
                    false)) {}

#define VIEW_EVENT_CASE(eventType)                      \
  case CONSTEXPR_RAW_PROPS_KEY_HASH("on" #eventType): { \
    const auto offset = ViewEvents::Offset::eventType;  \
    ViewEvents defaultViewEvents{};                     \
    bool res = defaultViewEvents[offset];               \
    if (value.hasValue()) {                             \
      fromRawValue(context, value, res);                \
    }                                                   \
    events[offset] = res;                               \
    return;                                             \
  }

void BaseViewProps::setProp(
    const PropsParserContext& context,
    RawPropsPropNameHash hash,
    const char* propName,
    const RawValue& value) {
  // All Props structs setProp methods must always, unconditionally,
  // call all super::setProp methods, since multiple structs may
  // reuse the same values.
  YogaStylableProps::setProp(context, hash, propName, value);
  AccessibilityProps::setProp(context, hash, propName, value);

  static auto defaults = BaseViewProps{};

  switch (hash) {
    RAW_SET_PROP_SWITCH_CASE_BASIC(opacity);
    RAW_SET_PROP_SWITCH_CASE_BASIC(backgroundColor);
    RAW_SET_PROP_SWITCH_CASE_BASIC(backgroundImage);
    RAW_SET_PROP_SWITCH_CASE(backgroundImage, "experimental_backgroundImage");
    RAW_SET_PROP_SWITCH_CASE(backgroundSize, "experimental_backgroundSize");
    RAW_SET_PROP_SWITCH_CASE(
        backgroundPosition, "experimental_backgroundPosition");
    RAW_SET_PROP_SWITCH_CASE(backgroundRepeat, "experimental_backgroundRepeat");
    RAW_SET_PROP_SWITCH_CASE_BASIC(shadowColor);
    RAW_SET_PROP_SWITCH_CASE_BASIC(shadowOffset);
    RAW_SET_PROP_SWITCH_CASE_BASIC(shadowOpacity);
    RAW_SET_PROP_SWITCH_CASE_BASIC(shadowRadius);
    RAW_SET_PROP_SWITCH_CASE_BASIC(transform);
    RAW_SET_PROP_SWITCH_CASE_BASIC(transformOrigin);
    RAW_SET_PROP_SWITCH_CASE_BASIC(backfaceVisibility);
    RAW_SET_PROP_SWITCH_CASE_BASIC(shouldRasterize);
    RAW_SET_PROP_SWITCH_CASE_BASIC(zIndex);
    RAW_SET_PROP_SWITCH_CASE_BASIC(pointerEvents);
    RAW_SET_PROP_SWITCH_CASE_BASIC(isolation);
    RAW_SET_PROP_SWITCH_CASE_BASIC(hitSlop);
    RAW_SET_PROP_SWITCH_CASE_BASIC(onLayout);
    RAW_SET_PROP_SWITCH_CASE_BASIC(collapsable);
    RAW_SET_PROP_SWITCH_CASE_BASIC(collapsableChildren);
    RAW_SET_PROP_SWITCH_CASE_BASIC(removeClippedSubviews);
    RAW_SET_PROP_SWITCH_CASE_BASIC(cursor);
    RAW_SET_PROP_SWITCH_CASE_BASIC(outlineColor);
    RAW_SET_PROP_SWITCH_CASE_BASIC(outlineOffset);
    RAW_SET_PROP_SWITCH_CASE_BASIC(outlineStyle);
    RAW_SET_PROP_SWITCH_CASE_BASIC(outlineWidth);
    RAW_SET_PROP_SWITCH_CASE_BASIC(filter);
    RAW_SET_PROP_SWITCH_CASE_BASIC(boxShadow);
    RAW_SET_PROP_SWITCH_CASE_BASIC(mixBlendMode);
    // events field
    VIEW_EVENT_CASE(PointerEnter);
    VIEW_EVENT_CASE(PointerEnterCapture);
    VIEW_EVENT_CASE(PointerMove);
    VIEW_EVENT_CASE(PointerMoveCapture);
    VIEW_EVENT_CASE(PointerLeave);
    VIEW_EVENT_CASE(PointerLeaveCapture);
    VIEW_EVENT_CASE(PointerOver);
    VIEW_EVENT_CASE(PointerOverCapture);
    VIEW_EVENT_CASE(PointerOut);
    VIEW_EVENT_CASE(PointerOutCapture);
    VIEW_EVENT_CASE(Click);
    VIEW_EVENT_CASE(ClickCapture);
    VIEW_EVENT_CASE(PointerDown);
    VIEW_EVENT_CASE(PointerDownCapture);
    VIEW_EVENT_CASE(PointerUp);
    VIEW_EVENT_CASE(PointerUpCapture);
    VIEW_EVENT_CASE(GotPointerCapture);
    VIEW_EVENT_CASE(LostPointerCapture);
    VIEW_EVENT_CASE(MoveShouldSetResponder);
    VIEW_EVENT_CASE(MoveShouldSetResponderCapture);
    VIEW_EVENT_CASE(StartShouldSetResponder);
    VIEW_EVENT_CASE(StartShouldSetResponderCapture);
    VIEW_EVENT_CASE(ResponderGrant);
    VIEW_EVENT_CASE(ResponderReject);
    VIEW_EVENT_CASE(ResponderStart);
    VIEW_EVENT_CASE(ResponderEnd);
    VIEW_EVENT_CASE(ResponderRelease);
    VIEW_EVENT_CASE(ResponderMove);
    VIEW_EVENT_CASE(ResponderTerminate);
    VIEW_EVENT_CASE(ResponderTerminationRequest);
    VIEW_EVENT_CASE(ShouldBlockNativeResponder);
    VIEW_EVENT_CASE(TouchStart);
    VIEW_EVENT_CASE(TouchMove);
    VIEW_EVENT_CASE(TouchEnd);
    VIEW_EVENT_CASE(TouchCancel);
    // BorderRadii
    SET_CASCADED_RECTANGLE_CORNERS(borderRadii, "border", "Radius", value);
    SET_CASCADED_RECTANGLE_EDGES(borderColors, "border", "Color", value);
    SET_CASCADED_RECTANGLE_CORNERS(borderCurves, "border", "Curve", value);
    SET_CASCADED_RECTANGLE_EDGES(borderStyles, "border", "Style", value);
  }
}

#pragma mark - Convenience Methods

static BorderRadii ensureNoOverlap(const BorderRadii& radii, const Size& size) {
  // "Corner curves must not overlap: When the sum of any two adjacent border
  // radii exceeds the size of the border box, UAs must proportionally reduce
  // the used values of all border radii until none of them overlap."
  // Source: https://www.w3.org/TR/css-backgrounds-3/#corner-overlap

  float leftEdgeRadii = radii.topLeft.vertical + radii.bottomLeft.vertical;
  float topEdgeRadii = radii.topLeft.horizontal + radii.topRight.horizontal;
  float rightEdgeRadii = radii.topRight.vertical + radii.bottomRight.vertical;
  float bottomEdgeRadii =
      radii.bottomLeft.horizontal + radii.bottomRight.horizontal;

  float leftEdgeRadiiScale =
      (leftEdgeRadii > 0) ? std::min(size.height / leftEdgeRadii, (Float)1) : 0;
  float topEdgeRadiiScale =
      (topEdgeRadii > 0) ? std::min(size.width / topEdgeRadii, (Float)1) : 0;
  float rightEdgeRadiiScale = (rightEdgeRadii > 0)
      ? std::min(size.height / rightEdgeRadii, (Float)1)
      : 0;
  float bottomEdgeRadiiScale = (bottomEdgeRadii > 0)
      ? std::min(size.width / bottomEdgeRadii, (Float)1)
      : 0;

  return BorderRadii{
      .topLeft =
          {static_cast<float>(
               radii.topLeft.vertical *
               std::min(topEdgeRadiiScale, leftEdgeRadiiScale)),
           static_cast<float>(
               radii.topLeft.horizontal *
               std::min(topEdgeRadiiScale, leftEdgeRadiiScale))},
      .topRight =
          {static_cast<float>(
               radii.topRight.vertical *
               std::min(topEdgeRadiiScale, rightEdgeRadiiScale)),
           static_cast<float>(
               radii.topRight.horizontal *
               std::min(topEdgeRadiiScale, rightEdgeRadiiScale))},
      .bottomLeft =
          {static_cast<float>(
               radii.bottomLeft.vertical *
               std::min(bottomEdgeRadiiScale, leftEdgeRadiiScale)),
           static_cast<float>(
               radii.bottomLeft.horizontal *
               std::min(bottomEdgeRadiiScale, leftEdgeRadiiScale))},
      .bottomRight =
          {static_cast<float>(
               radii.bottomRight.vertical *
               std::min(bottomEdgeRadiiScale, rightEdgeRadiiScale)),
           static_cast<float>(
               radii.bottomRight.horizontal *
               std::min(bottomEdgeRadiiScale, rightEdgeRadiiScale))},
  };
}

// static BorderRadii radiiPercentToPoint(
//     const RectangleCorners<NumericValue>& radii,
//     const Size& size) {
//   return BorderRadii{
//       .topLeft =
//           {radii.topLeft.resolve(size.height),
//            radii.topLeft.resolve(size.width)},
//       .topRight =
//           {radii.topRight.resolve(size.height),
//            radii.topRight.resolve(size.width)},
//       .bottomLeft =
//           {radii.bottomLeft.resolve(size.height),
//            radii.bottomLeft.resolve(size.width)},
//       .bottomRight =
//           {radii.bottomRight.resolve(size.height),
//            radii.bottomRight.resolve(size.width)},
//   };
// }

CascadedBorderWidths BaseViewProps::getBorderWidths(
    const LayoutContext& layoutContext) const {
  auto resolveBorder = [&](auto edge) -> std::optional<Float> {
    auto borderWidth = yogaStyle.border(edge);
    if (borderWidth.isDynamic()) {
      auto callbackId = borderWidth.callbackId();
      if (calcExpressions.contains(callbackId)) {
        auto& entry = calcExpressions.at(callbackId);
        return std::visit(
            [&](const auto& e) {
              return std::optional<Float>{e.calc.resolve(
                  0.0f,
                  layoutContext.viewportSize.width,
                  layoutContext.viewportSize.height)};
            },
            entry);
      }
    }
    return optionalFloatFromYogaValue(yogaStyle.border(edge));
  };

  return CascadedBorderWidths{
      .left = resolveBorder(yoga::Edge::Left),
      .top = resolveBorder(yoga::Edge::Top),
      .right = resolveBorder(yoga::Edge::Right),
      .bottom = resolveBorder(yoga::Edge::Bottom),
      .start = resolveBorder(yoga::Edge::Start),
      .end = resolveBorder(yoga::Edge::End),
      .horizontal = resolveBorder(yoga::Edge::Horizontal),
      .vertical = resolveBorder(yoga::Edge::Vertical),
      .all = resolveBorder(yoga::Edge::All),
  };
}

BorderMetrics BaseViewProps::resolveBorderMetrics(
    const LayoutMetrics& layoutMetrics) const {
  return resolveBorderMetrics(layoutMetrics, LayoutContext{});
}

BorderMetrics BaseViewProps::resolveBorderMetrics(
    const LayoutMetrics& layoutMetrics,
    const LayoutContext& layoutContext) const {
  auto isRTL =
      bool{layoutMetrics.layoutDirection == LayoutDirection::RightToLeft};
  const auto resolveContext =
      DynamicResolveContext(layoutMetrics, layoutContext);
  const auto resolver = DynamicResolver(calcExpressions, resolveContext);
  const auto& size = layoutMetrics.frame.size;
  auto resolveRadius = [&](const DynamicPropertyId& id,
                           std::optional<ValueUnit> value,
                           float ref) -> std::optional<ValueUnit> {
    if (calcExpressions.contains(id)) {
      return ValueUnit{(float)resolver.resolveNumber(id, ref), UnitType::Point};
    }
    return value;
  };
  auto resolvedH =
      CascadedBorderRadii{
          .topLeft = resolveRadius(
              fnv1a("borderTopLeftRadius"), borderRadii.topLeft, size.height),
          .topRight = resolveRadius(
              fnv1a("borderTopRightRadius"), borderRadii.topRight, size.height),
          .bottomLeft = resolveRadius(
              fnv1a("borderBottomLeftRadius"),
              borderRadii.bottomLeft,
              size.height),
          .bottomRight = resolveRadius(
              fnv1a("borderBottomRightRadius"),
              borderRadii.bottomRight,
              size.height),
          .topStart = resolveRadius(
              fnv1a("borderTopStartRadius"), borderRadii.topStart, size.height),
          .topEnd = resolveRadius(
              fnv1a("borderTopEndRadius"), borderRadii.topEnd, size.height),
          .bottomStart = resolveRadius(
              fnv1a("borderBottomStartRadius"),
              borderRadii.bottomStart,
              size.height),
          .bottomEnd = resolveRadius(
              fnv1a("borderBottomEndRadius"),
              borderRadii.bottomEnd,
              size.height),
          .all = resolveRadius(
              fnv1a("borderAllRadius"), borderRadii.all, size.height),
          .endEnd = resolveRadius(
              fnv1a("borderEndEndRadius"), borderRadii.endEnd, size.height),
          .endStart = resolveRadius(
              fnv1a("borderEndStartRadius"), borderRadii.endStart, size.height),
          .startEnd = resolveRadius(
              fnv1a("borderStartEndRadius"), borderRadii.startEnd, size.height),
          .startStart = resolveRadius(
              fnv1a("borderStartStartRadius"),
              borderRadii.startStart,
              size.height)}
          .resolve(isRTL, ValueUnit{0.0f, UnitType::Point});
  auto resolvedW =
      CascadedBorderRadii{
          .topLeft = resolveRadius(
              fnv1a("borderTopLeftRadius"), borderRadii.topLeft, size.width),
          .topRight = resolveRadius(
              fnv1a("borderTopRightRadius"), borderRadii.topRight, size.width),
          .bottomLeft = resolveRadius(
              fnv1a("borderBottomLeftRadius"),
              borderRadii.bottomLeft,
              size.width),
          .bottomRight = resolveRadius(
              fnv1a("borderBottomRightRadius"),
              borderRadii.bottomRight,
              size.width),
          .topStart = resolveRadius(
              fnv1a("borderTopStartRadius"), borderRadii.topStart, size.width),
          .topEnd = resolveRadius(
              fnv1a("borderTopEndRadius"), borderRadii.topEnd, size.width),
          .bottomStart = resolveRadius(
              fnv1a("borderBottomStartRadius"),
              borderRadii.bottomStart,
              size.width),
          .bottomEnd = resolveRadius(
              fnv1a("borderBottomEndRadius"),
              borderRadii.bottomEnd,
              size.width),
          .all = resolveRadius(
              fnv1a("borderAllRadius"), borderRadii.all, size.width),
          .endEnd = resolveRadius(
              fnv1a("borderEndEndRadius"), borderRadii.endEnd, size.width),
          .endStart = resolveRadius(
              fnv1a("borderEndStartRadius"), borderRadii.endStart, size.width),
          .startEnd = resolveRadius(
              fnv1a("borderStartEndRadius"), borderRadii.startEnd, size.width),
          .startStart = resolveRadius(
              fnv1a("borderStartStartRadius"),
              borderRadii.startStart,
              size.width)}
          .resolve(isRTL, ValueUnit{0.0f, UnitType::Point});
  ;
  BorderRadii radii = {
      .topLeft = {resolvedH.topLeft.value, resolvedW.topLeft.value},
      .topRight = {resolvedH.topRight.value, resolvedW.topRight.value},
      .bottomLeft = {resolvedH.bottomLeft.value, resolvedW.bottomLeft.value},
      .bottomRight = {resolvedH.bottomRight.value, resolvedW.bottomRight.value},
  };

  return {
      .borderColors = borderColors.resolve(isRTL, {}),
      .borderWidths = layoutMetrics.borderWidth,
      .borderRadii = ensureNoOverlap(radii, layoutMetrics.frame.size),
      .borderCurves = borderCurves.resolve(isRTL, BorderCurve::Circular),
      .borderStyles = borderStyles.resolve(isRTL, BorderStyle::Solid),
  };
}

Transform BaseViewProps::resolveTransform(
    const LayoutMetrics& layoutMetrics,
    const LayoutContext& layoutContext) const {
  return resolveTransform(
      DynamicResolver{calcExpressions, {layoutMetrics, layoutContext}});
}

Transform BaseViewProps::resolveTransform(
    const DynamicResolver& resolver) const {
  return resolveTransform(resolver, transform, transformOrigin);
}

Transform BaseViewProps::resolveTransform(
    const DynamicResolver& resolver,
    const Transform& transform,
    const TransformOrigin& transformOrigin) {
  auto transformMatrix = Transform{};

  // transform is matrix
  if (transform.operations.size() == 1 &&
      transform.operations[0].type == TransformOperationType::Arbitrary) {
    transformMatrix = transform;
  } else {
    for (const auto& operation : transform.operations) {
      transformMatrix = transformMatrix *
          Transform::FromTransformOperation(operation, resolver, transform);
    }
  }

  if (transformOrigin.isSet()) {
    std::array<float, 3> translateOffsets = getTranslateForTransformOrigin(
        resolver.context.frameWidth(),
        resolver.context.frameHeight(),
        transformOrigin);
    transformMatrix =
        Transform::Translate(
            translateOffsets[0], translateOffsets[1], translateOffsets[2]) *
        transformMatrix *
        Transform::Translate(
            -translateOffsets[0], -translateOffsets[1], -translateOffsets[2]);
  }

  return transformMatrix;
}

bool BaseViewProps::getClipsContentToBounds() const {
  return yogaStyle.overflow() != yoga::Overflow::Visible;
}

#define SET_CALC_PROPERTY_BASE(props, fieldName, value) \
  if (props.count(#fieldName)) {                        \
    auto& entry = props[#fieldName];                    \
    if (entry.isString()) {                             \
      entry = value;                                    \
    }                                                   \
  }

#define SET_CALC_PROPERTY(fieldName, value) \
  SET_CALC_PROPERTY_BASE(props, fieldName, value)

#define SET_OPTIONAL_CALC_PROPERTY(fieldName, value) \
  if (value) {                                       \
    SET_CALC_PROPERTY(fieldName, *value);            \
  }

#define SET_RESOLVED_OPTIONAL_CALC_PROPERTY(fieldName, value) \
  if (value) {                                                \
    SET_CALC_PROPERTY(fieldName, resolver.resolve(*value));   \
  }

#define SET_CALC_PROPERTY_ARRAY(arrayName, index, fieldName, value) \
  if (props.count(#arrayName)) {                                    \
    auto& array = props[#arrayName];                                \
    if (array.isArray()) {                                          \
      if (array.size() > index) {                                   \
        SET_CALC_PROPERTY_BASE(                                     \
            array[index], fieldName, resolver.resolve(value))       \
      }                                                             \
    }                                                               \
  }

void BaseViewProps::resolveProperties(const DynamicResolver& resolver) {
  if (!needsToResolveStyleValues) {
    return;
  }

#ifdef RN_SERIALIZABLE_STATE
  [[maybe_unused]] auto p1 = folly::toJson(rawProps);
  auto resolved = getResolvedProps(resolver);
  [[maybe_unused]] auto p2 = folly::toJson(resolved);
  rawProps.update(resolved);
  [[maybe_unused]] auto p3 = folly::toJson(rawProps);
#endif

  resolver.resolve(fnv1a("opacity"), opacity);
  resolver.resolve(fnv1a("outlineOffset"), outlineOffset);
  outlineWidth = resolver.resolveNumber(fnv1a("outlineWidth"));
  resolver.resolve(fnv1a("shadowOpacity"), shadowOpacity);
  resolver.resolve(fnv1a("shadowRadius"), shadowRadius);

  // Resolve any dynamic calc() entries in boxShadow fields.
  for (size_t i = 0; i < boxShadow.size(); ++i) {
    auto& shadow = boxShadow[i];
    const auto prefix = std::string("boxShadow.") + std::to_string(i);
    resolver.resolve(fnv1a(prefix + ".offsetX"), shadow.offsetX);
    resolver.resolve(fnv1a(prefix + ".offsetY"), shadow.offsetY);
    resolver.resolve(fnv1a(prefix + ".blurRadius"), shadow.blurRadius);
    resolver.resolve(fnv1a(prefix + ".spreadDistance"), shadow.spreadDistance);
  }

  // Resolve dynamic calc values in background image gradients.
  for (auto& bgImage : backgroundImage) {
    auto resolveColorStops = [&](std::vector<ColorStop>& colorStops,
                                 float lineLength) {
      for (auto& stop : colorStops) {
        if (stop.position.isDynamic() && lineLength != 0.0f) {
          stop.position =
              resolver.resolveLengthPercentage(stop.position, lineLength);
        }
      }
    };

    if (auto* linear = std::get_if<LinearGradient>(&bgImage)) {
      float lineLength = linearGradientLineLength(
          linear->direction,
          resolver.context.frameWidth(),
          resolver.context.frameHeight());
      resolveColorStops(linear->colorStops, lineLength);
    } else if (auto* radial = std::get_if<RadialGradient>(&bgImage)) {
      // For radial gradients, color stop percentages are relative to the
      // gradient-line length (max radius), which isn't known until render time.
      // Pass 0 so dynamic calc values are resolved for their px/vw components;
      // pure-percentage stops stay as percentages and are resolved at render
      // time.
      resolveColorStops(radial->colorStops, 0.0f);

      auto& pos = radial->position;
      if (pos.top.has_value()) {
        *pos.top = resolver.resolveLengthPercentage(
            *pos.top, resolver.context.frameHeight());
      }
      if (pos.left.has_value()) {
        *pos.left = resolver.resolveLengthPercentage(
            *pos.left, resolver.context.frameWidth());
      }
      if (pos.right.has_value()) {
        *pos.right = resolver.resolveLengthPercentage(
            *pos.right, resolver.context.frameWidth());
      }
      if (pos.bottom.has_value()) {
        *pos.bottom = resolver.resolveLengthPercentage(
            *pos.bottom, resolver.context.frameHeight());
      }

      if (auto* dims = std::get_if<RadialGradientSize::Dimensions>(
              &radial->size.value)) {
        dims->x = resolver.resolveLengthPercentage(
            dims->x, resolver.context.frameWidth());
        dims->y = resolver.resolveLengthPercentage(
            dims->y, resolver.context.frameHeight());
      }
    }
  }

  for (auto& filterFunction : filter) {
    if (auto* parameter =
            std::get_if<UntypedNumericValue>(&filterFunction.parameters)) {
      auto resolvedValue = filterFunction.type == FilterType::Blur
          ? UntypedNumericValue::length(resolver.resolveLength(*parameter))
          : UntypedNumericValue::number(resolver.resolveNumber(*parameter));
      *parameter = resolvedValue;
      continue;
    }

    if (auto* dropShadowParams =
            std::get_if<DropShadowParams>(&filterFunction.parameters)) {
      dropShadowParams->offsetX = LengthValue::length(
          resolver.resolveLength(dropShadowParams->offsetX));
      dropShadowParams->offsetY = LengthValue::length(
          resolver.resolveLength(dropShadowParams->offsetY));
      dropShadowParams->standardDeviation = LengthValue::length(
          resolver.resolveLength(dropShadowParams->standardDeviation));
    }
  }

  for (auto& position : backgroundPosition) {
    if (position.top.has_value() && position.top.value().isDynamic()) {
      position.top.value() = UntypedNumericValue::length(
          resolver.resolveLengthOrPercentage(position.top.value(), 0.0f));
    }
    if (position.left.has_value() && position.left.value().isDynamic()) {
      position.left.value() = UntypedNumericValue::length(
          resolver.resolveLengthOrPercentage(position.left.value(), 0.0f));
    }
    if (position.right.has_value() && position.right.value().isDynamic()) {
      position.right.value() = UntypedNumericValue::length(
          resolver.resolveLengthOrPercentage(position.right.value(), 0.0f));
    }
    if (position.bottom.has_value() && position.bottom.value().isDynamic()) {
      position.bottom.value() = UntypedNumericValue::length(
          resolver.resolveLengthOrPercentage(position.bottom.value(), 0.0f));
    }
  }

  for (auto& size : backgroundSize) {
    if (auto lengthPercentage =
            std::get_if<BackgroundSizeLengthPercentage>(&size)) {
      if (auto x = std::get_if<UntypedNumericValue>(&lengthPercentage->x);
          x && x->isDynamic()) {
        *x = UntypedNumericValue::length(resolver.resolveLengthOrPercentage(
            *x, resolver.context.frameWidth()));
      }
      if (auto y = std::get_if<UntypedNumericValue>(&lengthPercentage->y);
          y && y->isDynamic()) {
        *y = UntypedNumericValue::length(resolver.resolveLengthOrPercentage(
            *y, resolver.context.frameHeight()));
      }
    }
  }

  for (auto& operation : transform.operations) {
    if (operation.x.isDynamic()) {
      operation.x =
          resolver.resolveAny(operation.x, resolver.context.frameWidth());
    }
    if (operation.y.isDynamic()) {
      operation.y =
          resolver.resolveAny(operation.y, resolver.context.frameHeight());
    }
    if (operation.z.isDynamic()) {
      operation.z = resolver.resolveAny(operation.z, 0.0f);
    }
  }
}

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic BaseViewProps::getResolvedProps(
    const DynamicResolver& resolver) const {
  if (!needsToResolveStyleValues) {
    return rawProps;
  }

  auto props = YogaStylableProps::getResolvedProps(resolver);
  [[maybe_unused]] auto before = folly::toJson(rawProps);
  if (calcExpressions.contains(fnv1a("opacity"))) {
    props["opacity"] = resolver.resolveNumber(fnv1a("opacity"));
  }
  if (calcExpressions.contains(fnv1a("outlineOffset"))) {
    props["outlineOffset"] = resolver.resolveNumber(fnv1a("outlineOffset"));
  }
  if (calcExpressions.contains(fnv1a("outlineWidth"))) {
    props["outlineWidth"] = resolver.resolveNumber(fnv1a("outlineWidth"));
  }
  if (calcExpressions.contains(fnv1a("shadowOpacity"))) {
    props["shadowOpacity"] = resolver.resolveNumber(fnv1a("shadowOpacity"));
  }
  if (calcExpressions.contains(fnv1a("shadowRadius"))) {
    props["shadowRadius"] = resolver.resolveNumber(fnv1a("shadowRadius"));
  }

  // boxShadow: always re-serialize when the sentinel is present (which is
  // whenever boxShadow is non-empty) so that rawProps string values (e.g.
  // "2px") are replaced with resolved floats before Android reads them.
  // Fields that have dynamic calc() entries are resolved via the resolver;
  // plain float fields use the already-parsed value from the C++ vector.
  if (calcExpressions.contains(fnv1a("boxShadow"))) {
    for (size_t i = 0; i < boxShadow.size(); ++i) {
      auto& shadowEntry = props["boxShadow"][i];
      const auto prefix = std::string("boxShadow.") + std::to_string(i);
      shadowEntry["offsetX"] =
          resolver.resolveNumber(fnv1a(prefix + ".offsetX"));
      shadowEntry["offsetY"] =
          resolver.resolveNumber(fnv1a(prefix + ".offsetY"));
      shadowEntry["blurRadius"] =
          resolver.resolveNumber(fnv1a(prefix + ".blurRadius"));
      shadowEntry["spreadDistance"] =
          resolver.resolveNumber(fnv1a(prefix + ".spreadDistance"));
    }
  }

  auto findFilterEntry = [&](const std::string& typeKey) -> folly::dynamic* {
    if (props.count("filter")) {
      auto& array = props["filter"];
      if (array.isArray()) {
        for (size_t i = 0; i < array.size(); i++) {
          auto& entry = array[i];
          if (entry.isObject()) {
            if (entry.count(typeKey)) {
              return &entry[typeKey];
            }
          }
        }
      }
    }
    return nullptr;
  };

  for (const auto& filterFunction : filter) {
    auto entry = findFilterEntry(toString(filterFunction.type));
    if (!entry) {
      continue;
    }
    if (auto* parameter =
            std::get_if<UntypedNumericValue>(&filterFunction.parameters)) {
      if (parameter->isDynamic()) {
        *entry = filterFunction.type == FilterType::Blur
            ? resolver.toDynamicLength(*parameter)
            : resolver.toDynamicNumber(*parameter);
      }
    } else if (
        auto* dropShadowParams =
            std::get_if<DropShadowParams>(&filterFunction.parameters)) {
      if (dropShadowParams->offsetX.isDynamic()) {
        (*entry)["offsetX"] =
            resolver.toDynamicLength(dropShadowParams->offsetX);
      }
      if (dropShadowParams->offsetY.isDynamic()) {
        (*entry)["offsetY"] =
            resolver.toDynamicLength(dropShadowParams->offsetY);
      }
      if (dropShadowParams->standardDeviation.isDynamic()) {
        (*entry)["standardDeviation"] =
            resolver.toDynamicLength(dropShadowParams->standardDeviation);
      }
    }
  }

  if (props.count("experimental_backgroundImage")) {
    for (size_t i = 0; i < backgroundImage.size(); i++) {
      auto& bgImage = backgroundImage[i];
      auto& bgImageEntry = props["experimental_backgroundImage"][i];

      if (const auto* linear = std::get_if<LinearGradient>(&bgImage)) {
        float lineLength = linearGradientLineLength(
            linear->direction,
            resolver.context.frameWidth(),
            resolver.context.frameHeight());

        for (size_t j = 0; j < linear->colorStops.size(); j++) {
          const auto& stop = linear->colorStops[j];
          if (stop.position.isDynamic() && lineLength != 0.0f) {
            bgImageEntry["colorStops"][j]["position"] =
                resolver.toDynamicLengthOrPercentage(stop.position, lineLength);
          }
        }
      } else if (const auto* radial = std::get_if<RadialGradient>(&bgImage)) {
        for (size_t j = 0; j < radial->colorStops.size(); j++) {
          const auto& stop = radial->colorStops[j];
          if (stop.position.isDynamic()) {
            bgImageEntry["colorStops"][j]["position"] =
                resolver.toDynamicLengthOrPercentage(stop.position, 0.0f);
          }
        }

        if (radial->position.top.has_value() &&
            radial->position.top->isDynamic()) {
          bgImageEntry["position"]["top"] =
              resolver.toDynamicLengthOrPercentage(
                  *radial->position.top, resolver.context.frameHeight());
        }
        if (radial->position.left.has_value() &&
            radial->position.left->isDynamic()) {
          bgImageEntry["position"]["left"] =
              resolver.toDynamicLengthOrPercentage(
                  *radial->position.left, resolver.context.frameWidth());
        }
        if (radial->position.right.has_value() &&
            radial->position.right->isDynamic()) {
          bgImageEntry["position"]["right"] =
              resolver.toDynamicLengthOrPercentage(
                  *radial->position.right, resolver.context.frameWidth());
        }
        if (radial->position.bottom.has_value() &&
            radial->position.bottom->isDynamic()) {
          bgImageEntry["position"]["bottom"] =
              resolver.toDynamicLengthOrPercentage(
                  *radial->position.bottom, resolver.context.frameHeight());
        }

        if (const auto* dims = std::get_if<RadialGradientSize::Dimensions>(
                &radial->size.value)) {
          if (dims->x.isDynamic()) {
            bgImageEntry["size"]["x"] = resolver.toDynamicLengthOrPercentage(
                dims->x, resolver.context.frameWidth());
          }
          if (dims->y.isDynamic()) {
            bgImageEntry["size"]["y"] = resolver.toDynamicLengthOrPercentage(
                dims->y, resolver.context.frameHeight());
          }
        }
        props["experimental_backgroundImage"][i] = bgImageEntry;
      }
    }
  }

  if (calcExpressions.contains(fnv1a("borderRadius"))) {
    props["borderRadius"] = resolver.resolveNumber(fnv1a("borderRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderTopLeftRadius"))) {
    props["borderTopLeftRadius"] =
        resolver.resolveNumber(fnv1a("borderTopLeftRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderTopRightRadius"))) {
    props["borderTopRightRadius"] =
        resolver.resolveNumber(fnv1a("borderTopRightRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderBottomRightRadius"))) {
    props["borderBottomRightRadius"] =
        resolver.resolveNumber(fnv1a("borderBottomRightRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderBottomLeftRadius"))) {
    auto v = resolver.resolveNumber(fnv1a("borderBottomLeftRadius"), 0.0f);
    ;
    props["borderBottomLeftRadius"] = v;
  }
  if (calcExpressions.contains(fnv1a("borderTopStartRadius"))) {
    props["borderTopStartRadius"] =
        resolver.resolveNumber(fnv1a("borderTopStartRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderTopEndRadius"))) {
    props["borderTopEndRadius"] =
        resolver.resolveNumber(fnv1a("borderTopEndRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderBottomStartRadius"))) {
    props["borderBottomStartRadius"] =
        resolver.resolveNumber(fnv1a("borderBottomStartRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderBottomEndRadius"))) {
    props["borderBottomEndRadius"] =
        resolver.resolveNumber(fnv1a("borderBottomEndRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderEndEndRadius"))) {
    props["borderEndEndRadius"] =
        resolver.resolveNumber(fnv1a("borderEndEndRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderEndStartRadius"))) {
    props["borderEndStartRadius"] =
        resolver.resolveNumber(fnv1a("borderEndStartRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderStartEndRadius"))) {
    props["borderStartEndRadius"] =
        resolver.resolveNumber(fnv1a("borderStartEndRadius"));
  }
  if (calcExpressions.contains(fnv1a("borderStartStartRadius"))) {
    props["borderStartStartRadius"] =
        resolver.resolveNumber(fnv1a("borderStartStartRadius"));
  }

  [[maybe_unused]] auto after = folly::toJson(props);

  LOG(ERROR) << "Before: " << before;
  LOG(ERROR) << "After: " << after;

  return props;
}
#endif

#pragma mark - DebugStringConvertible

#if RN_DEBUG_STRING_CONVERTIBLE
SharedDebugStringConvertibleList BaseViewProps::getDebugProps() const {
  const auto& defaultBaseViewProps = BaseViewProps();

  return AccessibilityProps::getDebugProps() +
      YogaStylableProps::getDebugProps() +
      SharedDebugStringConvertibleList{
          debugStringConvertibleItem(
              "opacity", opacity, defaultBaseViewProps.opacity),
          debugStringConvertibleItem(
              "backgroundColor",
              backgroundColor,
              defaultBaseViewProps.backgroundColor),
          debugStringConvertibleItem(
              "zIndex", zIndex, defaultBaseViewProps.zIndex.value_or(0)),
          debugStringConvertibleItem(
              "pointerEvents",
              pointerEvents,
              defaultBaseViewProps.pointerEvents),
          debugStringConvertibleItem(
              "transform", transform, defaultBaseViewProps.transform),
          debugStringConvertibleItem(
              "backgroundImage",
              backgroundImage,
              defaultBaseViewProps.backgroundImage),
      };
}
#endif

void BaseViewProps::collectLiveResolvableIds(
    const DynamicPropertiesMap& map,
    std::unordered_set<DynamicPropertyId>& ids) const {
  YogaStylableProps::collectLiveResolvableIds(map, ids);

  auto addNV = [&](const auto& value) {
    if (value.isDynamic()) {
      ids.insert(value.asDynamicId());
    }
  };
  auto addOptionalNV = [&](const auto& value) {
    if (value && value->isDynamic()) {
      ids.insert(value->asDynamicId());
    }
  };
  auto addNVDirect = [&](const auto& value) {
    if (value.isDynamic()) {
      ids.insert(value.asDynamicId());
    }
  };
  auto addById = [&](const auto& id) {
    if (calcExpressions.contains(id)) {
      ids.insert(id);
    }
  };
  addById(fnv1a("opacity"));
  addById(fnv1a("outlineOffset"));
  addById(fnv1a("outlineWidth"));
  addById(fnv1a("shadowOpacity"));
  addById(fnv1a("shadowRadius"));

  // Sentinel: ensures needsToResolveStyleValues=true whenever boxShadow is
  // non-empty so that getResolvedProps re-serializes parsed Float values over
  // any rawProps string entries (e.g. "2px").
  addById(fnv1a("boxShadow"));
  // Per-field: keep any dynamic calc() entries alive across
  // sweepCalcExpressions.
  for (size_t i = 0; i < boxShadow.size(); ++i) {
    const auto prefix = std::string("boxShadow.") + std::to_string(i);
    addById(fnv1a((prefix + ".offsetX").c_str()));
    addById(fnv1a((prefix + ".offsetY").c_str()));
    addById(fnv1a((prefix + ".blurRadius").c_str()));
    addById(fnv1a((prefix + ".spreadDistance").c_str()));
  }

  addById(fnv1a("borderTopLeftRadius"));
  addById(fnv1a("borderTopRightRadius"));
  addById(fnv1a("borderBottomLeftRadius"));
  addById(fnv1a("borderBottomRightRadius"));
  addById(fnv1a("borderTopStartRadius"));
  addById(fnv1a("borderTopEndRadius"));
  addById(fnv1a("borderBottomStartRadius"));
  addById(fnv1a("borderBottomEndRadius"));
  addById(fnv1a("borderAllRadius"));
  addById(fnv1a("borderEndEndRadius"));
  addById(fnv1a("borderEndStartRadius"));
  addById(fnv1a("borderStartEndRadius"));
  addById(fnv1a("borderStartStartRadius"));

  addNVDirect(transformOrigin.xy[0]);
  addNVDirect(transformOrigin.xy[1]);

  for (const auto& op : transform.operations) {
    addNVDirect(op.x);
    addNVDirect(op.y);
    addNVDirect(op.z);
  }

  for (const auto& filterFunction : filter) {
    if (const auto* parameter =
            std::get_if<UntypedNumericValue>(&filterFunction.parameters)) {
      addNV(*parameter);
      continue;
    }

    if (const auto* dropShadowParams =
            std::get_if<DropShadowParams>(&filterFunction.parameters)) {
      addNV(dropShadowParams->offsetX);
      addNV(dropShadowParams->offsetY);
      addNV(dropShadowParams->standardDeviation);
    }
  }

  for (const auto& bp : backgroundPosition) {
    addOptionalNV(bp.top);
    addOptionalNV(bp.left);
    addOptionalNV(bp.right);
    addOptionalNV(bp.bottom);
  }

  for (const auto& bsVar : backgroundSize) {
    if (const auto* lp = std::get_if<BackgroundSizeLengthPercentage>(&bsVar)) {
      if (const auto* value = std::get_if<UntypedNumericValue>(&lp->x)) {
        addNVDirect(*value);
      }
      if (const auto* value = std::get_if<UntypedNumericValue>(&lp->y)) {
        addNVDirect(*value);
      }
    }
  }

  for (const auto& bgImage : backgroundImage) {
    auto collectColorStops = [&](const std::vector<ColorStop>& colorStops) {
      for (const auto& stop : colorStops) {
        addNV(stop.position);
      }
    };

    if (const auto* linear = std::get_if<LinearGradient>(&bgImage)) {
      collectColorStops(linear->colorStops);
    } else if (const auto* radial = std::get_if<RadialGradient>(&bgImage)) {
      collectColorStops(radial->colorStops);
      addOptionalNV(radial->position.top);
      addOptionalNV(radial->position.left);
      addOptionalNV(radial->position.right);
      addOptionalNV(radial->position.bottom);
      if (const auto* dims = std::get_if<RadialGradientSize::Dimensions>(
              &radial->size.value)) {
        addNVDirect(dims->x);
        addNVDirect(dims->y);
      }
    }
  }
}

} // namespace facebook::react
