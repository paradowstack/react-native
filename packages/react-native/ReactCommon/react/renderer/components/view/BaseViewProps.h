/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/AccessibilityProps.h>
#include <react/renderer/components/view/YogaStylableProps.h>
#include <react/renderer/components/view/primitives.h>
#include <react/renderer/core/LayoutContext.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/graphics/BackgroundImage.h>
#include <react/renderer/graphics/BackgroundPosition.h>
#include <react/renderer/graphics/BackgroundRepeat.h>
#include <react/renderer/graphics/BackgroundSize.h>
#include <react/renderer/graphics/BlendMode.h>
#include <react/renderer/graphics/BoxShadow.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Filter.h>
#include <react/renderer/graphics/Isolation.h>
#include <react/renderer/graphics/NumericValue.h>
#include <react/renderer/graphics/Transform.h>

#include <optional>

namespace facebook::react {

/**
 * Surface-wide and node-local inputs required to resolve environment-dependent
 * style values at the platform boundary.
 */

class BaseViewProps : public YogaStylableProps, public AccessibilityProps {
 public:
  BaseViewProps() = default;
  BaseViewProps(
      const PropsParserContext& context,
      const BaseViewProps& sourceProps,
      const RawProps& rawProps,
      const std::function<bool(const std::string&)>& filterObjectKeys =
          nullptr);

  void setProp(
      const PropsParserContext& context,
      RawPropsPropNameHash hash,
      const char* propName,
      const RawValue& value);

#pragma mark - Props

  // Color
  NumberValue opacity{NumberValue::number(1.0f)};
  SharedColor backgroundColor{};

  // Borders
  CascadedBorderRadii borderRadii{};
  CascadedBorderColors borderColors{};
  CascadedBorderCurves borderCurves{}; // iOS only?
  CascadedBorderStyles borderStyles{};

  // Outline
  SharedColor outlineColor{};
  LengthValue outlineOffset{LengthValue::length(0.0f)};
  OutlineStyle outlineStyle{OutlineStyle::Solid};
  LengthValue outlineWidth{LengthValue::length(0.0f)};

  // Shadow
  SharedColor shadowColor{};
  Size shadowOffset{0, -3};
  NumberValue shadowOpacity{NumberValue::number(0.0f)};
  LengthValue shadowRadius{LengthValue::length(3.0f)};

  Cursor cursor{};

  // Box shadow
  std::vector<BoxShadow> boxShadow{};

  // Filter
  std::vector<FilterFunction> filter{};

  // Background Image
  std::vector<BackgroundImage> backgroundImage{};

  // Background Size
  std::vector<BackgroundSize> backgroundSize{};

  // Background Position
  std::vector<BackgroundPosition> backgroundPosition{};

  // Background Repeat
  std::vector<BackgroundRepeat> backgroundRepeat{};

  // MixBlendMode
  BlendMode mixBlendMode{BlendMode::Normal};

  // Isolate
  Isolation isolation{Isolation::Auto};

  // Transform
  Transform transform{};
  TransformOrigin transformOrigin{};
  BackfaceVisibility backfaceVisibility{};
  bool shouldRasterize{};
  std::optional<int> zIndex{};

  // Events
  PointerEventsMode pointerEvents{};
  EdgeInsets hitSlop{};
  bool onLayout{};

  ViewEvents events{};

  bool collapsable{true};
  bool collapsableChildren{true};

  bool removeClippedSubviews{false};

#pragma mark - Convenience Methods

  CascadedBorderWidths getBorderWidths(
      const LayoutContext& layoutContext) const;
  BorderMetrics resolveBorderMetrics(const LayoutMetrics& layoutMetrics) const;
  BorderMetrics resolveBorderMetrics(
      const LayoutMetrics& layoutMetrics,
      const LayoutContext& layoutContext) const;
  Transform resolveTransform(const LayoutMetrics& layoutMetrics) const;
  bool getClipsContentToBounds() const;

  void resolveProperties(const DynamicResolver& resolver) override;
  void collectLiveResolvableIds(
      std::unordered_set<DynamicPropertyId>& ids) const override;

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic getResolvedProps(
      const DynamicResolver& resolver) const override;
#endif

 public:
  static Transform resolveTransform(
      const Size& frameSize,
      const Transform& transform,
      const TransformOrigin& transformOrigin);

#if RN_DEBUG_STRING_CONVERTIBLE
  SharedDebugStringConvertibleList getDebugProps() const override;
#endif
};

} // namespace facebook::react
