/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/ValueUnit.h>
#include <optional>
#include <sstream>

#ifdef RN_SERIALIZABLE_STATE
#include <folly/dynamic.h>
#endif

namespace facebook::react {

struct CircleShape {
  ValueUnit r{};
  std::optional<ValueUnit> cx{};
  std::optional<ValueUnit> cy{};

  bool operator==(const CircleShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct EllipseShape {
  ValueUnit rx{};
  ValueUnit ry{};
  std::optional<ValueUnit> cx{};
  std::optional<ValueUnit> cy{};

  bool operator==(const EllipseShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct InsetShape {
  ValueUnit top{};
  ValueUnit right{};
  ValueUnit bottom{};
  ValueUnit left{};
  std::optional<ValueUnit> borderRadius{};

  bool operator==(const InsetShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct PolygonShape {
  std::vector<std::pair<ValueUnit, ValueUnit>> points;

  bool operator==(const PolygonShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct RectShape {
  ValueUnit top{};
  ValueUnit right{};
  ValueUnit bottom{};
  ValueUnit left{};
  std::optional<ValueUnit> borderRadius{};

  bool operator==(const RectShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct XywhShape {
  ValueUnit x{};
  ValueUnit y{};
  ValueUnit width{};
  ValueUnit height{};
  std::optional<ValueUnit> borderRadius{};

  bool operator==(const XywhShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

struct PathShape {
  std::string pathData;

  bool operator==(const PathShape &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  void toString(std::stringstream &ss) const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

using BasicShape = std::variant<CircleShape, EllipseShape, InsetShape, PolygonShape, RectShape, XywhShape, PathShape>;

enum class GeometryBox : uint8_t {
  MarginBox,
  BorderBox,
  ContentBox,
  PaddingBox,
  FillBox,
  StrokeBox,
  ViewBox,
};

struct ClipPath {
  std::optional<BasicShape> shape;
  std::optional<GeometryBox> geometryBox;

  bool operator==(const ClipPath &other) const;

#if RN_DEBUG_STRING_CONVERTIBLE
  std::string toString() const;
#endif

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif
};

} // namespace facebook::react
