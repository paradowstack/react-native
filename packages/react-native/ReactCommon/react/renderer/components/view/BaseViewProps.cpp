/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "BaseViewProps.h"

#include <folly/json.h>
#include <algorithm>

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

namespace facebook::react {

namespace {

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
              : convertRawProp(
                    context,
                    rawProps,
                    "opacity",
                    sourceProps.opacity,
                    NumberValue::number(1.0f))),
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
                    "border",
                    "Radius",
                    sourceProps.borderRadii,
                    {})),
      borderColors(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderColors
              : convertRawProp(
                    context,
                    rawProps,
                    "border",
                    "Color",
                    sourceProps.borderColors,
                    {})),
      borderCurves(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderCurves
              : convertRawProp(
                    context,
                    rawProps,
                    "border",
                    "Curve",
                    sourceProps.borderCurves,
                    {})),
      borderStyles(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.borderStyles
              : convertRawProp(
                    context,
                    rawProps,
                    "border",
                    "Style",
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
              : convertRawProp(
                    context,
                    rawProps,
                    "outlineOffset",
                    sourceProps.outlineOffset,
                    LengthValue::length(0.0f))),
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
              : convertRawProp(
                    context,
                    rawProps,
                    "outlineWidth",
                    sourceProps.outlineWidth,
                    LengthValue::length(0.0f))),
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
              : convertRawProp(
                    context,
                    rawProps,
                    "shadowOpacity",
                    sourceProps.shadowOpacity,
                    NumberValue::number(0.0f))),
      shadowRadius(
          ReactNativeFeatureFlags::enableCppPropsIteratorSetter()
              ? sourceProps.shadowRadius
              : convertRawProp(
                    context,
                    rawProps,
                    "shadowRadius",
                    sourceProps.shadowRadius,
                    LengthValue::length(3.0f))),
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
              : convertRawProp(
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
              : convertRawProp(
                    context,
                    rawProps,
                    "experimental_backgroundImage",
                    sourceProps.backgroundImage,
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
        auto& calcExpression = calcExpressions.at(callbackId);
        return calcExpression.resolve(
            0.0f,
            layoutContext.viewportSize.width,
            layoutContext.viewportSize.height);
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
  auto resolved =
      borderRadii.resolve(isRTL, LengthPercentageValue::length(0.0f));
  auto resolveRadius = [&](const LengthPercentageValue& value,
                           float ref) -> float {
    return resolver.resolveLengthOrPercentage(value, ref);
  };
  const auto& size = layoutMetrics.frame.size;
  BorderRadii radii = {
      .topLeft =
          {resolveRadius(resolved.topLeft, size.height),
           resolveRadius(resolved.topLeft, size.width)},
      .topRight =
          {resolveRadius(resolved.topRight, size.height),
           resolveRadius(resolved.topRight, size.width)},
      .bottomLeft =
          {resolveRadius(resolved.bottomLeft, size.height),
           resolveRadius(resolved.bottomLeft, size.width)},
      .bottomRight =
          {resolveRadius(resolved.bottomRight, size.height),
           resolveRadius(resolved.bottomRight, size.width)},
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
    const LayoutMetrics& layoutMetrics) const {
  const auto& frameSize = layoutMetrics.frame.size;
  return resolveTransform(frameSize, transform, transformOrigin);
}

Transform BaseViewProps::resolveTransform(
    const Size& frameSize,
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
          Transform::FromTransformOperation(operation, frameSize, transform);
    }
  }

  if (transformOrigin.isSet()) {
    std::array<float, 3> translateOffsets = getTranslateForTransformOrigin(
        frameSize.width, frameSize.height, transformOrigin);
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
  auto resolved = getResolvedProps(resolver);
  rawProps.update(resolved);
#endif

  opacity = NumberValue::number(resolver.resolveNumber(opacity));
  outlineOffset = LengthValue::length(resolver.resolveLength(outlineOffset));
  outlineWidth = LengthValue::length(resolver.resolveLength(outlineWidth));
  shadowOpacity = NumberValue::number(resolver.resolveNumber(shadowOpacity));
  shadowRadius = LengthValue::length(resolver.resolveLength(shadowRadius));

  // Resolve length-like values in box shadows.
  for (auto& shadow : boxShadow) {
    shadow.offsetX =
        LengthValue::length(resolver.resolveLength(shadow.offsetX));
    shadow.offsetY =
        LengthValue::length(resolver.resolveLength(shadow.offsetY));
    shadow.blurRadius =
        LengthValue::length(resolver.resolveLength(shadow.blurRadius));
    shadow.spreadDistance =
        LengthValue::length(resolver.resolveLength(shadow.spreadDistance));
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
        *x = UntypedNumericValue::length(
            resolver.resolveLengthOrPercentage(*x, 0.0f));
      }
      if (auto y = std::get_if<UntypedNumericValue>(&lengthPercentage->y);
          y && y->isDynamic()) {
        *y = UntypedNumericValue::length(
            resolver.resolveLengthOrPercentage(*y, 0.0f));
      }
    }
  }

  sweepCalcExpressions();
}

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic BaseViewProps::getResolvedProps(
    const DynamicResolver& resolver) const {
  folly::dynamic props = rawProps;
  if (!needsToResolveStyleValues) {
    return props;
  }

  [[maybe_unused]] auto before = folly::toJson(rawProps);
  if (opacity.isDynamic()) {
    props["opacity"] = resolver.toDynamicNumber(opacity);
  }
  if (outlineOffset.isDynamic()) {
    props["outlineOffset"] = resolver.toDynamicLength(outlineOffset);
  }
  if (outlineWidth.isDynamic()) {
    props["outlineWidth"] = resolver.toDynamicLength(outlineWidth);
  }
  if (shadowOpacity.isDynamic()) {
    props["shadowOpacity"] = resolver.toDynamicNumber(shadowOpacity);
  }
  if (shadowRadius.isDynamic()) {
    props["shadowRadius"] = resolver.toDynamicLength(shadowRadius);
  }

  // Resolve length-like values in box shadows.
  for (size_t i = 0; i < boxShadow.size(); i++) {
    auto& shadow = boxShadow[i];
    auto& shadowEntry = props["boxShadow"][i];
    if (shadow.offsetX.isDynamic()) {
      shadowEntry["offsetX"] = resolver.toDynamicLength(shadow.offsetX);
    }
    if (shadow.offsetY.isDynamic()) {
      shadowEntry["offsetY"] = resolver.toDynamicLength(shadow.offsetY);
    }
    if (shadow.blurRadius.isDynamic()) {
      shadowEntry["blurRadius"] = resolver.toDynamicLength(shadow.blurRadius);
    }
    if (shadow.spreadDistance.isDynamic()) {
      shadowEntry["spreadDistance"] =
          resolver.toDynamicLength(shadow.spreadDistance);
    }
    props["boxShadow"][i] = shadowEntry;
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

  auto borderWidths = getBorderWidths(resolver.context.layoutContext);
  if (borderWidths.all.has_value()) {
    props["borderWidth"] = borderWidths.all.value();
  }
  if (borderWidths.left.has_value()) {
    props["borderLeftWidth"] = borderWidths.left.value();
  }
  if (borderWidths.right.has_value()) {
    props["borderRightWidth"] = borderWidths.right.value();
  }
  if (borderWidths.top.has_value()) {
    props["borderTopWidth"] = borderWidths.top.value();
  }
  if (borderWidths.bottom.has_value()) {
    props["borderBottomWidth"] = borderWidths.bottom.value();
  }
  if (borderWidths.start.has_value()) {
    props["borderStartWidth"] = borderWidths.start.value();
  }
  if (borderWidths.end.has_value()) {
    props["borderEndWidth"] = borderWidths.end.value();
  }
  if (borderWidths.horizontal.has_value()) {
    props["borderHorizontalWidth"] = borderWidths.horizontal.value();
  }
  if (borderWidths.vertical.has_value()) {
    props["borderVerticalWidth"] = borderWidths.vertical.value();
  }

  if (borderRadii.all.has_value() && borderRadii.all.value().isDynamic()) {
    props["borderRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.all.value());
  }
  if (borderRadii.topLeft.has_value() &&
      borderRadii.topLeft.value().isDynamic()) {
    props["borderTopLeftRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.topLeft.value());
  }
  if (borderRadii.topRight.has_value() &&
      borderRadii.topRight.value().isDynamic()) {
    props["borderTopRightRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.topRight.value());
  }
  if (borderRadii.bottomRight.has_value() &&
      borderRadii.bottomRight.value().isDynamic()) {
    props["borderBottomRightRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.bottomRight.value());
  }
  if (borderRadii.bottomLeft.has_value() &&
      borderRadii.bottomLeft.value().isDynamic()) {
    props["borderBottomLeftRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.bottomLeft.value());
  }
  if (borderRadii.topStart.has_value() &&
      borderRadii.topStart.value().isDynamic()) {
    props["borderTopStartRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.topStart.value());
  }
  if (borderRadii.topEnd.has_value() &&
      borderRadii.topEnd.value().isDynamic()) {
    props["borderTopEndRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.topEnd.value());
  }
  if (borderRadii.bottomStart.has_value() &&
      borderRadii.bottomStart.value().isDynamic()) {
    props["borderBottomStartRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.bottomStart.value());
  }
  if (borderRadii.bottomEnd.has_value() &&
      borderRadii.bottomEnd.value().isDynamic()) {
    props["borderBottomEndRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.bottomEnd.value());
  }
  if (borderRadii.endEnd.has_value() &&
      borderRadii.endEnd.value().isDynamic()) {
    props["borderEndEndRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.endEnd.value());
  }
  if (borderRadii.endStart.has_value() &&
      borderRadii.endStart.value().isDynamic()) {
    props["borderEndStartRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.endStart.value());
  }
  if (borderRadii.startEnd.has_value() &&
      borderRadii.startEnd.value().isDynamic()) {
    props["borderStartEndRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.startEnd.value());
  }
  if (borderRadii.startStart.has_value() &&
      borderRadii.startStart.value().isDynamic()) {
    props["borderStartStartRadius"] =
        resolver.toDynamicLengthOrPercentage(borderRadii.startStart.value());
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
    std::unordered_set<DynamicPropertyId>& ids) const {
  YogaStylableProps::collectLiveResolvableIds(ids);

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

  addNV(opacity);
  addNV(outlineOffset);
  addNV(outlineWidth);
  addNV(shadowOpacity);
  addNV(shadowRadius);

  addOptionalNV(borderRadii.topLeft);
  addOptionalNV(borderRadii.topRight);
  addOptionalNV(borderRadii.bottomLeft);
  addOptionalNV(borderRadii.bottomRight);
  addOptionalNV(borderRadii.topStart);
  addOptionalNV(borderRadii.topEnd);
  addOptionalNV(borderRadii.bottomStart);
  addOptionalNV(borderRadii.bottomEnd);
  addOptionalNV(borderRadii.all);
  addOptionalNV(borderRadii.endEnd);
  addOptionalNV(borderRadii.endStart);
  addOptionalNV(borderRadii.startEnd);
  addOptionalNV(borderRadii.startStart);

  addNVDirect(transformOrigin.xy[0]);
  addNVDirect(transformOrigin.xy[1]);

  for (const auto& op : transform.operations) {
    addNVDirect(op.x);
    addNVDirect(op.y);
    addNVDirect(op.z);
  }

  for (const auto& bs : boxShadow) {
    addNV(bs.offsetX);
    addNV(bs.offsetY);
    addNV(bs.blurRadius);
    addNV(bs.spreadDistance);
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
}

} // namespace facebook::react
