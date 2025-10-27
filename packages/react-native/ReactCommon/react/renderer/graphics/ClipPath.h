/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <optional>
#include <sstream>

#ifdef RN_SERIALIZABLE_STATE
#include <folly/dynamic.h>
#endif

namespace facebook::react {

struct Circle {
  Float cx{};
  Float cy{};
  Float r{};

  bool operator==(const Circle& other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream& ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct Ellipse {
  Float cx{};
  Float cy{};
  Float rx{};
  Float ry{};

  bool operator==(const Ellipse& other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream& ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct Rectangle {
  Float x{};
  Float y{};
  Float width{};
  Float height{};

  bool operator==(const Rectangle& other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream& ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

using BasicShape = std::variant<Circle, Ellipse, Rectangle>;

enum GeometryBox {
  MarginBox,
  BorderBox,
  ContentBox,
  PaddingBox,
  FillBox,
  StrokeBox,
  ViewBox,
};

using ClipPath = std::variant<BasicShape, GeometryBox>;

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic toDynamic(const ClipPath& clipPath);
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
std::string toString(std::vector<ClipPath>& value);
#endif

} // namespace facebook::react
