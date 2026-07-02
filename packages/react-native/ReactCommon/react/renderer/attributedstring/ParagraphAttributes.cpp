/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ParagraphAttributes.h"

#include <react/renderer/attributedstring/conversions.h>
#include <react/renderer/core/graphicsConversions.h>
#include <react/renderer/debug/debugStringConvertibleUtils.h>
#include <react/utils/FloatComparison.h>

namespace facebook::react {

bool ParagraphAttributes::operator==(const ParagraphAttributes& rhs) const {
  return std::tie(
             maximumNumberOfLines,
             ellipsizeMode,
             textBreakStrategy,
             adjustsFontSizeToFit,
             includeFontPadding,
             android_hyphenationFrequency,
             textAlignVertical) ==
      std::tie(
             rhs.maximumNumberOfLines,
             rhs.ellipsizeMode,
             rhs.textBreakStrategy,
             rhs.adjustsFontSizeToFit,
             rhs.includeFontPadding,
             rhs.android_hyphenationFrequency,
             rhs.textAlignVertical) &&
      floatEquality(minimumFontSize, rhs.minimumFontSize) &&
      floatEquality(maximumFontSize, rhs.maximumFontSize) &&
      floatEquality(minimumFontScale, rhs.minimumFontScale);
}

#pragma mark - DebugStringConvertible

#if RN_DEBUG_STRING_CONVERTIBLE
SharedDebugStringConvertibleList ParagraphAttributes::getDebugProps() const {
  ParagraphAttributes paragraphAttributes{};
  return {
      debugStringConvertibleItem(
          "maximumNumberOfLines",
          maximumNumberOfLines,
          paragraphAttributes.maximumNumberOfLines),
      debugStringConvertibleItem(
          "ellipsizeMode", ellipsizeMode, paragraphAttributes.ellipsizeMode),
      debugStringConvertibleItem(
          "textBreakStrategy",
          textBreakStrategy,
          paragraphAttributes.textBreakStrategy),
      debugStringConvertibleItem(
          "adjustsFontSizeToFit",
          adjustsFontSizeToFit,
          paragraphAttributes.adjustsFontSizeToFit),
      debugStringConvertibleItem(
          "minimumFontSize",
          minimumFontSize,
          paragraphAttributes.minimumFontSize),
      debugStringConvertibleItem(
          "maximumFontSize",
          maximumFontSize,
          paragraphAttributes.maximumFontSize),
      debugStringConvertibleItem(
          "includeFontPadding",
          includeFontPadding,
          paragraphAttributes.includeFontPadding),
      debugStringConvertibleItem(
          "android_hyphenationFrequency",
          android_hyphenationFrequency,
          paragraphAttributes.android_hyphenationFrequency),
      debugStringConvertibleItem(
          "textAlignVertical",
          textAlignVertical,
          paragraphAttributes.textAlignVertical)};
}
#endif

void ParagraphAttributes::resolveProperties(const DynamicResolver& resolver) {
  resolver.resolve(fnv1a("minimumFontSize"), minimumFontSize);
  resolver.resolve(fnv1a("maximumFontSize"), maximumFontSize);
  resolver.resolve(fnv1a("minimumFontScale"), minimumFontScale);
}

void ParagraphAttributes::collectLiveResolvableIds(
    const DynamicPropertiesMap& map,
    std::unordered_set<DynamicPropertyId>& ids) const {
  auto addById = [&](auto id) {
    if (map.contains(id))
      ids.insert(id);
  };
  addById(fnv1a("minimumFontSize"));
  addById(fnv1a("maximumFontSize"));
  addById(fnv1a("minimumFontScale"));
}

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic ParagraphAttributes::getResolvedProps(
    const DynamicResolver& resolver) const {
  folly::dynamic props = folly::dynamic::object();
  if (resolver.propertiesMap.contains(fnv1a("minimumFontSize"))) {
    props["minimumFontSize"] = resolver.resolveNumber(fnv1a("minimumFontSize"));
  }
  if (resolver.propertiesMap.contains(fnv1a("maximumFontSize"))) {
    props["maximumFontSize"] = resolver.resolveNumber(fnv1a("maximumFontSize"));
  }
  if (resolver.propertiesMap.contains(fnv1a("minimumFontScale"))) {
    props["minimumFontScale"] =
        resolver.resolveNumber(fnv1a("minimumFontScale"));
  }
  return props;
}
#endif

} // namespace facebook::react
