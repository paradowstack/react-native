/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ClipPathPropsConversions.h"
#include <glog/logging.h>
#include <react/debug/react_native_expect.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/components/view/CSSConversions.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/css/CSSClipPath.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/renderer/graphics/ClipPath.h>
#include <unordered_map>

namespace facebook::react {

void parseProcessedClipPath(
    const PropsParserContext& context,
    const RawValue& value,
    std::optional<ClipPath>& result) {
  // react_native_expect(value.hasType<std::vector<RawValue>>());
  // if (!value.hasType<std::vector<RawValue>>()) {
  //   result = {};
  //   return;
  // }

  // std::vector<ClipPath> ClipPaths{};
  // auto rawClipPaths = static_cast<std::vector<RawValue>>(value);
  // for (const auto& rawClipPath : rawClipPaths) {
  //   bool isMap =
  //       rawClipPath.hasType<std::unordered_map<std::string, RawValue>>();
  //   react_native_expect(isMap);
  //   if (!isMap) {
  //     // If any box shadow is malformed then we should not apply any of them
  //     // which is the web behavior.
  //     result = {};
  //     return;
  //   }

  //   auto rawClipPathMap =
  //       static_cast<std::unordered_map<std::string, RawValue>>(rawClipPath);
  //   ClipPath ClipPath{};
  //   auto offsetX = rawClipPathMap.find("offsetX");
  //   react_native_expect(offsetX != rawClipPathMap.end());
  //   if (offsetX == rawClipPathMap.end()) {
  //     result = {};
  //     return;
  //   }
  //   react_native_expect(offsetX->second.hasType<Float>());
  //   if (!offsetX->second.hasType<Float>()) {
  //     result = {};
  //     return;
  //   }
  //   ClipPath.offsetX = (Float)offsetX->second;

  //   auto offsetY = rawClipPathMap.find("offsetY");
  //   react_native_expect(offsetY != rawClipPathMap.end());
  //   if (offsetY == rawClipPathMap.end()) {
  //     result = {};
  //     return;
  //   }
  //   react_native_expect(offsetY->second.hasType<Float>());
  //   if (!offsetY->second.hasType<Float>()) {
  //     result = {};
  //     return;
  //   }
  //   ClipPath.offsetY = (Float)offsetY->second;

  //   auto blurRadius = rawClipPathMap.find("blurRadius");
  //   if (blurRadius != rawClipPathMap.end()) {
  //     react_native_expect(blurRadius->second.hasType<Float>());
  //     if (!blurRadius->second.hasType<Float>()) {
  //       result = {};
  //       return;
  //     }
  //     ClipPath.blurRadius = (Float)blurRadius->second;
  //   }

  //   auto spreadDistance = rawClipPathMap.find("spreadDistance");
  //   if (spreadDistance != rawClipPathMap.end()) {
  //     react_native_expect(spreadDistance->second.hasType<Float>());
  //     if (!spreadDistance->second.hasType<Float>()) {
  //       result = {};
  //       return;
  //     }
  //     ClipPath.spreadDistance = (Float)spreadDistance->second;
  //   }

  //   auto inset = rawClipPathMap.find("inset");
  //   if (inset != rawClipPathMap.end()) {
  //     react_native_expect(inset->second.hasType<bool>());
  //     if (!inset->second.hasType<bool>()) {
  //       result = {};
  //       return;
  //     }
  //     ClipPath.inset = (bool)inset->second;
  //   }

  //   auto color = rawClipPathMap.find("color");
  //   if (color != rawClipPathMap.end()) {
  //     fromRawValue(
  //         context.contextContainer,
  //         context.surfaceId,
  //         color->second,
  //         ClipPath.color);
  //   }

  //   ClipPaths.push_back(ClipPath);
  // }

  ClipPath path;
  result = path;
}

// std::optional<ClipPath> fromCSSShadow(const CSSShadow& cssShadow) {
//   return {};
//  // TODO: handle non-px values
//  if (cssShadow.offsetX.unit != CSSLengthUnit::Px ||
//      cssShadow.offsetY.unit != CSSLengthUnit::Px ||
//      cssShadow.blurRadius.unit != CSSLengthUnit::Px ||
//      cssShadow.spreadDistance.unit != CSSLengthUnit::Px) {
//    return {};
//  }

// return ClipPath{
//     .offsetX = cssShadow.offsetX.value,
//     .offsetY = cssShadow.offsetY.value,
//     .blurRadius = cssShadow.blurRadius.value,
//     .spreadDistance = cssShadow.spreadDistance.value,
//     .color = fromCSSColor(cssShadow.color),
//     .inset = cssShadow.inset,
// };
//}

void parseUnprocessedClipPathString(
    std::string&& value,
    std::optional<ClipPath>& result) {
  auto clipPath = parseCSSProperty<CSSShadowList>((std::string)value);
  if (!std::holds_alternative<CSSShadowList>(clipPath)) {
    result = {};
    return;
  }

  for (const auto& cssShadow : std::get<CSSShadowList>(clipPath)) {
    if (auto ClipPath = fromCSSShadow(cssShadow)) {
      result.push_back(*ClipPath);
    } else {
      result = {};
      return;
    }
  }
}

std::optional<ClipPath> parseClipPathRawValue(
    const PropsParserContext& context,
    const RawValue& value) {
  return {};
  // if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
  //   return {};
  // }

  // auto ClipPath = std::unordered_map<std::string, RawValue>(value);
  // auto rawOffsetX = ClipPath.find("offsetX");
  // if (rawOffsetX == ClipPath.end()) {
  //   return {};
  // }
  // auto offsetX = coerceLength(rawOffsetX->second);
  // if (!offsetX.has_value()) {
  //   return {};
  // }

  // auto rawOffsetY = ClipPath.find("offsetY");
  // if (rawOffsetY == ClipPath.end()) {
  //   return {};
  // }
  // auto offsetY = coerceLength(rawOffsetY->second);
  // if (!offsetY.has_value()) {
  //   return {};
  // }

  // Float blurRadius = 0;
  // auto rawBlurRadius = ClipPath.find("blurRadius");
  // if (rawBlurRadius != ClipPath.end()) {
  //   if (auto blurRadiusValue = coerceLength(rawBlurRadius->second)) {
  //     if (*blurRadiusValue < 0) {
  //       return {};
  //     }
  //     blurRadius = *blurRadiusValue;
  //   } else {
  //     return {};
  //   }
  // }

  // Float spreadDistance = 0;
  // auto rawSpreadDistance = ClipPath.find("spreadDistance");
  // if (rawSpreadDistance != ClipPath.end()) {
  //   if (auto spreadDistanceValue = coerceLength(rawSpreadDistance->second)) {
  //     spreadDistance = *spreadDistanceValue;
  //   } else {
  //     return {};
  //   }
  // }

  // bool inset = false;
  // auto rawInset = ClipPath.find("inset");
  // if (rawInset != ClipPath.end()) {
  //   if (rawInset->second.hasType<bool>()) {
  //     inset = (bool)rawInset->second;
  //   } else {
  //     return {};
  //   }
  // }

  // SharedColor color;
  // auto rawColor = ClipPath.find("color");
  // if (rawColor != ClipPath.end()) {
  //   color = coerceColor(rawColor->second, context);
  //   if (!color) {
  //     return {};
  //   }
  // }

  // return ClipPath{
  //     .offsetX = *offsetX,
  //     .offsetY = *offsetY,
  //     .blurRadius = blurRadius,
  //     .spreadDistance = spreadDistance,
  //     .color = color,
  //     .inset = inset};
}

void parseUnprocessedClipPathList(
    const PropsParserContext& context,
    std::vector<RawValue>&& value,
    std::optional<ClipPath>& result) {
  // for (const auto& rawValue : value) {
  //   if (auto ClipPath = parseClipPathRawValue(context, rawValue)) {
  //     result.push_back(*ClipPath);
  //   } else {
  //     result = {};
  //     return;
  //   }
  // }

  ClipPath path;
  result = path;
}

} // namespace facebook::react
