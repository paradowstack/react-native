/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ClipPath.h"

#include <folly/dynamic.h>
#include <sstream>
#include <string>
#include <vector>

namespace facebook::react {

bool CircleShape::operator==(const CircleShape& other) const {
  return cx == other.cx && cy == other.cy && r == other.r;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void CircleShape::toString(std::stringstream& ss) const {
  ss << "circle(" << r << "px";
  if (cx || cy) {
    ss << " at ";
    if (cx) {
      ss << cx->toString();
    }
    if (cy) {
      ss << " " << cy->toString();
    }
  }
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic CircleShape::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["r"] = r.toDynamic();
  if (cx) {
    result["cx"] = cx->toDynamic();
  }
  if (cy) {
    result["cy"] = cy->toDynamic();
  }
  return result;
}
#endif

bool EllipseShape::operator==(const EllipseShape& other) const {
  return cx == other.cx && cy == other.cy && rx == other.rx && ry == other.ry;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void EllipseShape::toString(std::stringstream& ss) const {
  ss << "ellipse(" << rx << "px " << ry << "px";
  if (cx || cy) {
    ss << " at ";
    if (cx) {
      ss << cx->toString();
    }
    if (cy) {
      ss << " " << cy->toString();
    }
  }
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic EllipseShape::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["rx"] = r.toDynamic();
  result["ry"] = r.toDynamic();
  if (cx) {
    result["cx"] = cx->toDynamic();
  }
  if (cy) {
    result["cy"] = cy->toDynamic();
  }
  return result;
}
#endif

bool InsetShape::operator==(const InsetShape& other) const {
  return top == other.top && right == other.right && bottom == other.bottom &&
      left == other.left && borderRadius == other.borderRadius;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void InsetShape::toString(std::stringstream& ss) const {
  ss << "inset(" << top.toString() << " " << right.toString() << " "
     << bottom.toString() << " " << left.toString();
  if (borderRadius) {
    ss << " round " << borderRadius->toString();
  }
  ss << ")";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic InsetShape::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["top"] = top.toDynamic();
  result["right"] = right.toDynamic();
  result["bottom"] = bottom.toDynamic();
  result["left"] = left.toDynamic();
  return result;
}
#endif

bool PolygonShape::operator==(const PolygonShape& other) const {
  return points == other.points;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void PolygonShape::toString(std::stringstream& ss) const {
  ss << "polygon(";
  for (size_t i = 0; i < points.size(); i++) {
    if (i > 0) {
      ss << ", ";
    }
    ss << points[i].first.toString() << " " << points[i].second.toString();
  }
  ss << ")";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic PolygonShape::toDynamic() const {
  folly::dynamic result = folly::dynamic::array();
  for (const auto& point : points) {
    folly::dynamic pointObj = folly::dynamic::object();
    pointObj["x"] = point.first.toDynamic();
    pointObj["y"] = point.second.toDynamic();
    result.push_back(pointObj);
  }
  return result;
}
#endif

// RectShape implementations
bool RectShape::operator==(const RectShape& other) const {
  return top == other.top && right == other.right && bottom == other.bottom &&
      left == other.left && borderRadius == other.borderRadius;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void RectShape::toString(std::stringstream& ss) const {
  ss << "rect(" << top.toString() << " " << right.toString() << " "
     << bottom.toString() << " " << left.toString() << " ";
  if (borderRadius) {
    ss << "round " << borderRadius->toString();
  }
  ss << ")";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic RectShape::toDynamic() const {
  return folly::dynamic::object()("top", top.toDynamic())(
      "right", right.toDynamic())("bottom", bottom.toDynamic())(
      "left", left.toDynamic());
}
#endif

// XywhShape implementations
bool XywhShape::operator==(const XywhShape& other) const {
  return x == other.x && y == other.y && width == other.width &&
      height == other.height && borderRadius == other.borderRadius;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void XywhShape::toString(std::stringstream& ss) const {
  ss << "xywh(" << x.toString() << " " << y.toString() << " "
     << width.toString() << " " << height.toString();
  if (borderRadius) {
    ss << " round " << borderRadius->toString();
  }
  ss << ")";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic XywhShape::toDynamic() const {
  return folly::dynamic::object()("x", x.toDynamic())("y", y.toDynamic())(
      "width", width.toDynamic())("height", height.toDynamic());
}
#endif

// PathShape implementations
bool PathShape::operator==(const PathShape& other) const {
  return pathData == other.pathData;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void PathShape::toString(std::stringstream& ss) const {
  ss << "path(\"" << pathData << "\")";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic PathShape::toDynamic() const {
  return folly::dynamic::object()("pathData", pathData);
}
#endif

bool ClipPath::operator==(const ClipPath& other) const {
  return shape == other.shape && geometryBox == other.geometryBox;
}

#if RN_DEBUG_STRING_CONVERTIBLE
std::string ClipPath::toString() const {
  std::stringstream ss;

  if (shape) {
    std::visit([&](const auto& s) { s.toString(ss); }, *shape);
  }

  if (geometryBox) {
    if (shape) {
      ss << " ";
    }
    switch (*geometryBox) {
      case GeometryBox::MarginBox:
        ss << "margin-box";
        break;
      case GeometryBox::BorderBox:
        ss << "border-box";
        break;
      case GeometryBox::ContentBox:
        ss << "content-box";
        break;
      case GeometryBox::PaddingBox:
        ss << "padding-box";
        break;
      case GeometryBox::FillBox:
        ss << "fill-box";
        break;
      case GeometryBox::StrokeBox:
        ss << "stroke-box";
        break;
      case GeometryBox::ViewBox:
        ss << "view-box";
        break;
    }
  }

  return ss.str();
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic ClipPath::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();

  if (shape) {
    result["shape"] =
        std::visit([](const auto& s) { return s.toDynamic(); }, *shape);
  }

  if (geometryBox) {
    switch (*geometryBox) {
      case GeometryBox::MarginBox:
        result["geometryBox"] = "margin-box";
        break;
      case GeometryBox::BorderBox:
        result["geometryBox"] = "border-box";
        break;
      case GeometryBox::ContentBox:
        result["geometryBox"] = "content-box";
        break;
      case GeometryBox::PaddingBox:
        result["geometryBox"] = "padding-box";
        break;
      case GeometryBox::FillBox:
        result["geometryBox"] = "fill-box";
        break;
      case GeometryBox::StrokeBox:
        result["geometryBox"] = "stroke-box";
        break;
      case GeometryBox::ViewBox:
        result["geometryBox"] = "view-box";
        break;
    }
  }

  return result;
}
#endif

} // namespace facebook::react
