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

bool Circle::operator==(const Circle& other) const {
  return cx == other.cx && cy == other.cy && r == other.r;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void Circle::toString(std::stringstream& ss) const {
  ss << "circle(" << r << "px at " << cx << "px " << cy << "px)";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic Circle::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["cx"] = cx;
  result["cy"] = cy;
  result["r"] = r;
  return result;
}
#endif

bool Ellipse::operator==(const Ellipse& other) const {
  return cx == other.cx && cy == other.cy && rx == other.rx && ry == other.ry;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void Ellipse::toString(std::stringstream& ss) const {
  ss << "ellipse(" << rx << "px " << ry << "px at " << cx << "px " << cy
     << "px)";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic Ellipse::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["cx"] = cx;
  result["cy"] = cy;
  result["rx"] = rx;
  result["ry"] = ry;
  return result;
}
#endif

bool Rectangle::operator==(const Rectangle& other) const {
  return x == other.x && y == other.y && width == other.width &&
      height == other.height;
}

#if RN_DEBUG_STRING_CONVERTIBLE
void Rectangle::toString(std::stringstream& ss) const {
  ss << "rectangle(" << x << "px " << y << "px " << width << "px " << height
     << "px)";
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic Rectangle::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["x"] = x;
  result["y"] = y;
  result["width"] = width;
  result["height"] = height;
  return result;
}
#endif

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic toDynamic(const ClipPath& clipPath) {
  if (std::holds_alternative<GeometryBox>(clipPath)) {
    folly::dynamic result = folly::dynamic::object();
    switch (std::get<GeometryBox>(clipPath)) {
      case MarginBox:
        result["type"] = "margin-box";
        break;
      case BorderBox:
        result["type"] = "border-box";
        break;
      case ContentBox:
        result["type"] = "content-box";
        break;
      case PaddingBox:
        result["type"] = "padding-box";
        break;
      case FillBox:
        result["type"] = "fill-box";
        break;
      case StrokeBox:
        result["type"] = "stroke-box";
        break;
      case ViewBox:
        result["type"] = "view-box";
        break;
    }
    return result;
  } else if (std::holds_alternative<BasicShape>(clipPath)) {
    return std::visit(
        [&](const auto& shape) { return shape.toDynamic(); },
        std::get<BasicShape>(clipPath));
  }
  return folly::dynamic(nullptr);
}
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
std::string toString(std::vector<ClipPath>& value) {
  std::stringstream ss;

  ss << "[";
  for (size_t i = 0; i < value.size(); i++) {
    if (i > 0) {
      ss << ", ";
    }

    const auto& clipPath = value[i];
    if (std::holds_alternative<GeometryBox>(clipPath)) {
      switch (std::get<GeometryBox>(clipPath)) {
        case MarginBox:
          ss << "margin-box";
          break;
        case BorderBox:
          ss << "border-box";
          break;
        case ContentBox:
          ss << "content-box";
          break;
        case PaddingBox:
          ss << "padding-box";
          break;
        case FillBox:
          ss << "fill-box";
          break;
        case StrokeBox:
          ss << "stroke-box";
          break;
        case ViewBox:
          ss << "view-box";
          break;
      }
    } else if (std::holds_alternative<BasicShape>(clipPath)) {
      std::visit(
          [&](const auto& shape) { shape.toString(ss); },
          std::get<BasicShape>(clipPath));
    }
  }
  ss << "]";

  return ss.str();
}
#endif

} // namespace facebook::react
