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
#include <react/renderer/components/view/conversions.h>
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
      if (cssPolygon.fillRule == CSSFillRule::NonZero) {
        polygon.fillRule = FillRule::NonZero;
      } else if (cssPolygon.fillRule == CSSFillRule::EvenOdd) {
        polygon.fillRule = FillRule::EvenOdd;
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
  if (!value.hasType<std::unordered_map<std::string, RawValue>>()) {
    result = {};
    return;
  }

  auto rawClipPath =
      static_cast<std::unordered_map<std::string, RawValue>>(value);
  ClipPath clipPath;

  // Parse shape if present
  auto shapeIt = rawClipPath.find("shape");
  if (shapeIt != rawClipPath.end() &&
      shapeIt->second.hasType<std::unordered_map<std::string, RawValue>>()) {
    auto rawShape =
        static_cast<std::unordered_map<std::string, RawValue>>(shapeIt->second);

    auto typeIt = rawShape.find("type");
    if (typeIt == rawShape.end() || !typeIt->second.hasType<std::string>()) {
      result = {};
      return;
    }

    std::string type = (std::string)(typeIt->second);

    if (type == "inset") {
      InsetShape inset;

      auto topIt = rawShape.find("top");
      if (topIt != rawShape.end()) {
        inset.top = toValueUnit(topIt->second);
      }

      auto rightIt = rawShape.find("right");
      if (rightIt != rawShape.end()) {
        inset.right = toValueUnit(rightIt->second);
      }

      auto bottomIt = rawShape.find("bottom");
      if (bottomIt != rawShape.end()) {
        inset.bottom = toValueUnit(bottomIt->second);
      }

      auto leftIt = rawShape.find("left");
      if (leftIt != rawShape.end()) {
        inset.left = toValueUnit(leftIt->second);
      }

      auto borderRadiusIt = rawShape.find("borderRadius");
      if (borderRadiusIt != rawShape.end()) {
        inset.borderRadius = toValueUnit(borderRadiusIt->second);
      }

      clipPath.shape = inset;
    } else if (type == "circle") {
      CircleShape circle;

      auto rIt = rawShape.find("r");
      if (rIt != rawShape.end()) {
        circle.r = toValueUnit(rIt->second);
      }

      auto cxIt = rawShape.find("cx");
      if (cxIt != rawShape.end()) {
        circle.cx = toValueUnit(cxIt->second);
      }

      auto cyIt = rawShape.find("cy");
      if (cyIt != rawShape.end()) {
        circle.cy = toValueUnit(cyIt->second);
      }

      clipPath.shape = circle;
    } else if (type == "ellipse") {
      EllipseShape ellipse;

      auto rxIt = rawShape.find("rx");
      if (rxIt != rawShape.end()) {
        ellipse.rx = toValueUnit(rxIt->second);
      }

      auto ryIt = rawShape.find("ry");
      if (ryIt != rawShape.end()) {
        ellipse.ry = toValueUnit(ryIt->second);
      }

      auto cxIt = rawShape.find("cx");
      if (cxIt != rawShape.end()) {
        ellipse.cx = toValueUnit(cxIt->second);
      }

      auto cyIt = rawShape.find("cy");
      if (cyIt != rawShape.end()) {
        ellipse.cy = toValueUnit(cyIt->second);
      }

      clipPath.shape = ellipse;
    } else if (type == "polygon") {
      PolygonShape polygon;

      auto pointsIt = rawShape.find("points");
      if (pointsIt != rawShape.end() &&
          pointsIt->second.hasType<std::vector<RawValue>>()) {
        auto rawPoints = static_cast<std::vector<RawValue>>(pointsIt->second);
        for (const auto& rawPoint : rawPoints) {
          if (rawPoint.hasType<std::unordered_map<std::string, RawValue>>()) {
            auto pointMap =
                static_cast<std::unordered_map<std::string, RawValue>>(
                    rawPoint);
            auto xIt = pointMap.find("x");
            auto yIt = pointMap.find("y");

            if (xIt != pointMap.end() && yIt != pointMap.end()) {
              polygon.points.push_back(
                  {toValueUnit(xIt->second), toValueUnit(yIt->second)});
            }
          }
        }
      }

      auto fillRuleIt = rawShape.find("fillRule");
      if (fillRuleIt != rawShape.end() &&
          fillRuleIt->second.hasType<std::string>()) {
        std::string fillRule = (std::string)(fillRuleIt->second);
        if (fillRule == "nonzero") {
          polygon.fillRule = FillRule::NonZero;
        } else if (fillRule == "evenodd") {
          polygon.fillRule = FillRule::EvenOdd;
        }
      }

      clipPath.shape = polygon;
    } else if (type == "rect") {
      RectShape rect;

      auto topIt = rawShape.find("top");
      if (topIt != rawShape.end()) {
        rect.top = toValueUnit(topIt->second);
      }

      auto rightIt = rawShape.find("right");
      if (rightIt != rawShape.end()) {
        rect.right = toValueUnit(rightIt->second);
      }

      auto bottomIt = rawShape.find("bottom");
      if (bottomIt != rawShape.end()) {
        rect.bottom = toValueUnit(bottomIt->second);
      }

      auto leftIt = rawShape.find("left");
      if (leftIt != rawShape.end()) {
        rect.left = toValueUnit(leftIt->second);
      }

      auto borderRadiusIt = rawShape.find("borderRadius");
      if (borderRadiusIt != rawShape.end()) {
        rect.borderRadius = toValueUnit(borderRadiusIt->second);
      }

      clipPath.shape = rect;
    } else if (type == "xywh") {
      XywhShape xywh;

      auto xIt = rawShape.find("x");
      if (xIt != rawShape.end()) {
        xywh.x = toValueUnit(xIt->second);
      }

      auto yIt = rawShape.find("y");
      if (yIt != rawShape.end()) {
        xywh.y = toValueUnit(yIt->second);
      }

      auto widthIt = rawShape.find("width");
      if (widthIt != rawShape.end()) {
        xywh.width = toValueUnit(widthIt->second);
      }

      auto heightIt = rawShape.find("height");
      if (heightIt != rawShape.end()) {
        xywh.height = toValueUnit(heightIt->second);
      }

      auto borderRadiusIt = rawShape.find("borderRadius");
      if (borderRadiusIt != rawShape.end()) {
        xywh.borderRadius = toValueUnit(borderRadiusIt->second);
      }

      clipPath.shape = xywh;
    } else {
      // Unknown shape type
      result = {};
      return;
    }
  }

  // Parse geometry box if present
  auto geometryBoxIt = rawClipPath.find("geometryBox");
  if (geometryBoxIt != rawClipPath.end() &&
      geometryBoxIt->second.hasType<std::string>()) {
    std::string geometryBox = (std::string)(geometryBoxIt->second);

    if (geometryBox == "border-box") {
      clipPath.geometryBox = GeometryBox::BorderBox;
    } else if (geometryBox == "padding-box") {
      clipPath.geometryBox = GeometryBox::PaddingBox;
    } else if (geometryBox == "content-box") {
      clipPath.geometryBox = GeometryBox::ContentBox;
    } else if (geometryBox == "margin-box") {
      clipPath.geometryBox = GeometryBox::MarginBox;
    } else if (geometryBox == "fill-box") {
      clipPath.geometryBox = GeometryBox::FillBox;
    } else if (geometryBox == "stroke-box") {
      clipPath.geometryBox = GeometryBox::StrokeBox;
    } else if (geometryBox == "view-box") {
      clipPath.geometryBox = GeometryBox::ViewBox;
    }
  }

  result = clipPath;
}

void parseUnprocessedClipPath(
    std::string&& value,
    std::optional<ClipPath>& result) {
  auto clipPath = parseCSSProperty<CSSClipPath>((std::string)value);
  if (std::holds_alternative<std::monostate>(clipPath)) {
    result = {};
    return;
  }

  result = fromCSSClipPath(std::get<CSSClipPath>(clipPath));
}

} // namespace facebook::react
