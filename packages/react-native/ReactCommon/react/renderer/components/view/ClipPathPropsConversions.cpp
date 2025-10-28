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
#include <unordered_map>

namespace facebook::react {

namespace {
ValueUnit convertLengthPercentageToValueUnit(
    const std::variant<CSSLength, CSSPercentage>& value) {
  if (std::holds_alternative<CSSLength>(value)) {
    return {std::get<CSSLength>(value).value, UnitType::Point};
  } else {
    return {std::get<CSSPercentage>(value).value, UnitType::Percent};
  }
}

GeometryBox convertCSSGeometryBox(CSSGeometryBox cssBox) {
  switch (cssBox) {
    case CSSGeometryBox::MarginBox:
      return GeometryBox::MarginBox;
    case CSSGeometryBox::BorderBox:
      return GeometryBox::BorderBox;
    case CSSGeometryBox::ContentBox:
      return GeometryBox::ContentBox;
    case CSSGeometryBox::PaddingBox:
      return GeometryBox::PaddingBox;
    case CSSGeometryBox::FillBox:
      return GeometryBox::FillBox;
    case CSSGeometryBox::StrokeBox:
      return GeometryBox::StrokeBox;
    case CSSGeometryBox::ViewBox:
      return GeometryBox::ViewBox;
  }
}
} // namespace

std::optional<ClipPath> fromCSSClipPath(const CSSClipPath& cssClipPath) {
  ClipPath result;

  // Convert shape if present
  if (cssClipPath.shape) {
    const auto& cssShape = *cssClipPath.shape;

    if (std::holds_alternative<CSSCircleShape>(cssShape)) {
      auto cssCircle = std::get<CSSCircleShape>(cssShape);
      CircleShape circle;
      if (cssCircle.radius) {
        circle.r = convertLengthPercentageToValueUnit(*cssCircle.radius);
      }
      if (cssCircle.cx) {
        circle.cx = convertLengthPercentageToValueUnit(*cssCircle.cx);
      }
      if (cssCircle.cy) {
        circle.cy = convertLengthPercentageToValueUnit(*cssCircle.cy);
      }
      result.shape = circle;
    } else if (std::holds_alternative<CSSEllipseShape>(cssShape)) {
      auto cssEllipse = std::get<CSSEllipseShape>(cssShape);
      EllipseShape ellipse;
      if (cssEllipse.rx) {
        ellipse.rx = convertLengthPercentageToValueUnit(*cssEllipse.rx);
      }
      if (cssEllipse.ry) {
        ellipse.ry = convertLengthPercentageToValueUnit(*cssEllipse.ry);
      }
      if (cssEllipse.cx) {
        ellipse.cx = convertLengthPercentageToValueUnit(*cssEllipse.cx);
      }
      if (cssEllipse.cy) {
        ellipse.cy = convertLengthPercentageToValueUnit(*cssEllipse.cy);
      }
      result.shape = ellipse;
    } else if (std::holds_alternative<CSSInsetShape>(cssShape)) {
      auto cssInset = std::get<CSSInsetShape>(cssShape);
      InsetShape inset;
      if (cssInset.top) {
        inset.top = convertLengthPercentageToValueUnit(*cssInset.top);
      }
      if (cssInset.right) {
        inset.right = convertLengthPercentageToValueUnit(*cssInset.right);
      }
      if (cssInset.bottom) {
        inset.bottom = convertLengthPercentageToValueUnit(*cssInset.bottom);
      }
      if (cssInset.left) {
        inset.left = convertLengthPercentageToValueUnit(*cssInset.left);
      }
      if (cssInset.borderRadius) {
        inset.borderRadius =
            convertLengthPercentageToValueUnit(*cssInset.borderRadius);
      }
      result.shape = inset;
    } else if (std::holds_alternative<CSSPolygonShape>(cssShape)) {
      auto cssPolygon = std::get<CSSPolygonShape>(cssShape);
      PolygonShape polygon;
      for (const auto& point : cssPolygon.points) {
        polygon.points.push_back(
            {convertLengthPercentageToValueUnit(point.first),
             convertLengthPercentageToValueUnit(point.second)});
      }
      result.shape = polygon;
    } else if (std::holds_alternative<CSSRectShape>(cssShape)) {
      auto cssRect = std::get<CSSRectShape>(cssShape);
      RectShape rect;
      rect.top = convertLengthPercentageToValueUnit(cssRect.top);
      rect.right = convertLengthPercentageToValueUnit(cssRect.right);
      rect.bottom = convertLengthPercentageToValueUnit(cssRect.bottom);
      rect.left = convertLengthPercentageToValueUnit(cssRect.left);
      if (cssRect.borderRadius) {
        rect.borderRadius =
            convertLengthPercentageToValueUnit(*cssRect.borderRadius);
      }
      result.shape = rect;
    } else if (std::holds_alternative<CSSXywhShape>(cssShape)) {
      auto cssXywh = std::get<CSSXywhShape>(cssShape);
      XywhShape xywh;
      xywh.x = convertLengthPercentageToValueUnit(cssXywh.x);
      xywh.y = convertLengthPercentageToValueUnit(cssXywh.y);
      xywh.width = convertLengthPercentageToValueUnit(cssXywh.width);
      xywh.height = convertLengthPercentageToValueUnit(cssXywh.height);
      if (cssXywh.borderRadius) {
        xywh.borderRadius =
            convertLengthPercentageToValueUnit(*cssXywh.borderRadius);
      }
      result.shape = xywh;
    } else if (std::holds_alternative<CSSPathShape>(cssShape)) {
      auto cssPath = std::get<CSSPathShape>(cssShape);
      PathShape path;
      path.pathData = cssPath.pathData;
      result.shape = path;
    }
  }

  // Convert geometry box if present
  if (cssClipPath.geometryBox) {
    result.geometryBox = convertCSSGeometryBox(*cssClipPath.geometryBox);
  }

  return result;
}

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
  auto clipPath = parseCSSProperty<CSSClipPath>((std::string)value);
  if (std::holds_alternative<std::monostate>(clipPath)) {
    result = {};
    return;
  }

  result = fromCSSClipPath(std::get<CSSClipPath>(clipPath));

  // std::vector<ClipPath> results;
  // auto filterList = parseCSSProperty<CSSClipPathList>((std::string)value);
  // if (!std::holds_alternative<CSSClipPathList>(filterList)) {
  //   results = {};
  //   return;
  // }

  // for (const auto& cssFilter : std::get<CSSClipPathList>(filterList)) {
  //   if (auto filter = fromCSSClipPath(cssFilter)) {
  //     results.push_back(*filter);
  // 		result = *filter;
  // 		return;
  //   } else {
  //     results = {};
  //     return;
  //   }
  // }
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
