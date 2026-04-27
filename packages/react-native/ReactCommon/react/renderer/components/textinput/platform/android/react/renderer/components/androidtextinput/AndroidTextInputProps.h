/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/Props.h>
#include <react/renderer/graphics/Color.h>

#include <react/renderer/attributedstring/TextAttributes.h>
#include <react/renderer/attributedstring/conversions.h>
#include <react/renderer/components/textinput/BaseTextInputProps.h>
#include <react/renderer/components/textinput/basePrimitives.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/propsConversions.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/NumericValue.h>
#include <react/renderer/imagemanager/primitives.h>
#include <unordered_map>

namespace facebook::react {

struct AndroidTextInputTextShadowOffsetStruct {
  double width;
  double height;
};

inline static bool operator==(
    const AndroidTextInputTextShadowOffsetStruct& lhs,
    const AndroidTextInputTextShadowOffsetStruct& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

static inline void fromRawValue(
    const PropsParserContext& context,
    const RawValue& value,
    AndroidTextInputTextShadowOffsetStruct& result) {
  auto map = (std::unordered_map<std::string, RawValue>)value;

  auto width = map.find("width");
  if (width != map.end()) {
    fromRawValue(context, width->second, result.width);
  }
  auto height = map.find("height");
  if (height != map.end()) {
    fromRawValue(context, height->second, result.height);
  }
}

static inline std::string toString(
    const AndroidTextInputTextShadowOffsetStruct& value) {
  return "[Object AndroidTextInputTextShadowOffsetStruct]";
}

inline folly::dynamic toDynamic(
    const AndroidTextInputTextShadowOffsetStruct& value) {
  folly::dynamic dynamicValue = folly::dynamic::object();
  dynamicValue["width"] = value.width;
  dynamicValue["height"] = value.height;
  return dynamicValue;
}

class AndroidTextInputProps final : public BaseTextInputProps {
 public:
  AndroidTextInputProps() = default;
  AndroidTextInputProps(
      const PropsParserContext& context,
      const AndroidTextInputProps& sourceProps,
      const RawProps& rawProps);

  void setProp(
      const PropsParserContext& context,
      RawPropsPropNameHash hash,
      const char* propName,
      const RawValue& value);

  folly::dynamic getDynamic() const;

#pragma mark - Props

  std::string autoComplete{};
  std::string returnKeyLabel{};
  int numberOfLines{0};
  bool disableFullscreenUI{false};
  std::string textBreakStrategy{};
  std::string inlineImageLeft{};
  int inlineImagePadding{0};
  std::string importantForAutofill{};
  bool showSoftInputOnFocus{false};
  bool autoCorrect{false};
  bool allowFontScaling{false};
  NumberValue maxFontSizeMultiplier{NumberValue::number(0.0f)};
  std::string keyboardType{};
  std::string returnKeyType{};
  bool secureTextEntry{false};
  std::string value{};
  bool selectTextOnFocus{false};
  bool caretHidden{false};
  bool contextMenuHidden{false};
  SharedColor textShadowColor{};
  LengthValue textShadowRadius{LengthValue::length(0.0f)};
  std::string textDecorationLine{};
  std::string fontStyle{};
  AndroidTextInputTextShadowOffsetStruct textShadowOffset{};
  LengthValue lineHeight{LengthValue::length(0.0f)};
  std::string textTransform{};
  SharedColor color{0};
  LengthValue letterSpacing{LengthValue::length(0.0f)};
  LengthValue fontSize{LengthValue::length(0.0f)};
  std::string textAlign{};
  bool includeFontPadding{false};
  std::string fontWeight{};
  std::string fontFamily{};

  /**
   * Auxiliary information to detect if these props are set or not.
   * See AndroidTextInputComponentDescriptor for usage.
   * TODO T63008435: can these, and this feature, be removed entirely?
   */
  bool hasPadding{};
  bool hasPaddingHorizontal{};
  bool hasPaddingVertical{};
  bool hasPaddingLeft{};
  bool hasPaddingTop{};
  bool hasPaddingRight{};
  bool hasPaddingBottom{};
  bool hasPaddingStart{};
  bool hasPaddingEnd{};

#if RN_DEBUG_STRING_CONVERTIBLE
  SharedDebugStringConvertibleList getDebugProps() const override;
#endif

  ComponentName getDiffPropsImplementationTarget() const override;
  folly::dynamic getDiffProps(
      const Props* prevProps,
      const LayoutMetrics* layoutMetrics = nullptr,
      const LayoutContext* layoutContext = nullptr) const override;

  void resolveProperties(const DynamicResolver& resolver) override;
  void collectLiveResolvableIds(
      std::unordered_set<DynamicPropertyId>& ids) const override;

  folly::dynamic getResolvedProps(
      const DynamicResolver& resolver) const override;
};

} // namespace facebook::react
