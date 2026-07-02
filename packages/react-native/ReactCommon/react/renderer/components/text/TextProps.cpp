/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextProps.h"

namespace facebook::react {

TextProps::TextProps(
    const PropsParserContext& context,
    const TextProps& sourceProps,
    const RawProps& rawProps)
    : Props(context, sourceProps, rawProps),
      BaseTextProps::BaseTextProps(context, sourceProps, rawProps) {};

void TextProps::setProp(
    const PropsParserContext& context,
    RawPropsPropNameHash hash,
    const char* propName,
    const RawValue& value) {
  BaseTextProps::setProp(context, hash, propName, value);
  Props::setProp(context, hash, propName, value);
}

#pragma mark - DebugStringConvertible

#if RN_DEBUG_STRING_CONVERTIBLE
SharedDebugStringConvertibleList TextProps::getDebugProps() const {
  return BaseTextProps::getDebugProps();
}
#endif

void TextProps::resolveProperties(const DynamicResolver& resolver) {
  if (!needsToResolveStyleValues) {
    return;
  }

  BaseTextProps::resolveProperties(resolver);
}
void TextProps::collectLiveResolvableIds(
    const DynamicPropertiesMap& map,
    std::unordered_set<DynamicPropertyId>& ids) const {
  BaseTextProps::collectLiveResolvableIds(map, ids);
}

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic TextProps::getResolvedProps(
    const DynamicResolver& resolver) const {
  return BaseTextProps::getResolvedProps(resolver);
}

ComponentName TextProps::getDiffPropsImplementationTarget() const {
  return "Text";
}

folly::dynamic TextProps::getDiffProps(
    const Props* prevProps,
    const LayoutMetrics* /*layoutMetrics*/,
    const LayoutContext* /*layoutContext*/) const {
  folly::dynamic result = folly::dynamic::object();

  static const auto defaultProps = TextProps();

  const TextProps* oldProps = prevProps == nullptr
      ? &defaultProps
      : static_cast<const TextProps*>(prevProps);

  BaseTextProps::appendTextAttributesProps(result, oldProps);

  return result;
}

#endif
} // namespace facebook::react
