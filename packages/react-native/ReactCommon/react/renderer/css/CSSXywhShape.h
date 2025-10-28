/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <variant>

#include <react/renderer/css/CSSDataType.h>
#include <react/renderer/css/CSSLength.h>
#include <react/renderer/css/CSSLengthPercentage.h>
#include <react/renderer/css/CSSPercentage.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/utils/iequals.h>

namespace facebook::react {

/**
 * Representation of CSS xywh() function
 * xywh(x, y, width, height)
 */
struct CSSXywhShape {
  std::variant<CSSLength, CSSPercentage> x;
  std::variant<CSSLength, CSSPercentage> y;
  std::variant<CSSLength, CSSPercentage> width;
  std::variant<CSSLength, CSSPercentage> height;

  bool operator==(const CSSXywhShape &other) const
  {
    return x == other.x && y == other.y && width == other.width && height == other.height;
  }
};

template <>
struct CSSDataTypeParser<CSSXywhShape> {
  static auto consumeFunctionBlock(const CSSFunctionBlock &func, CSSSyntaxParser &parser) -> std::optional<CSSXywhShape>
  {
    if (!iequals(func.name, "xywh")) {
      return {};
    }

    // Parse: x y width height (space-separated)
    auto x = parseNextCSSValue<CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(x)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto y = parseNextCSSValue<CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(y)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto width = parseNextCSSValue<CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(width)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto height = parseNextCSSValue<CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(height)) {
      return std::nullopt;
    }

    // Convert to variant<CSSLength, CSSPercentage>
    std::variant<CSSLength, CSSPercentage> xValue;
    std::variant<CSSLength, CSSPercentage> yValue;
    std::variant<CSSLength, CSSPercentage> widthValue;
    std::variant<CSSLength, CSSPercentage> heightValue;

    if (std::holds_alternative<CSSLength>(x)) {
      xValue = std::get<CSSLength>(x);
    } else if (std::holds_alternative<CSSPercentage>(x)) {
      xValue = std::get<CSSPercentage>(x);
    }

    if (std::holds_alternative<CSSLength>(y)) {
      yValue = std::get<CSSLength>(y);
    } else if (std::holds_alternative<CSSPercentage>(y)) {
      yValue = std::get<CSSPercentage>(y);
    }

    if (std::holds_alternative<CSSLength>(width)) {
      widthValue = std::get<CSSLength>(width);
    } else if (std::holds_alternative<CSSPercentage>(width)) {
      widthValue = std::get<CSSPercentage>(width);
    }

    if (std::holds_alternative<CSSLength>(height)) {
      heightValue = std::get<CSSLength>(height);
    } else if (std::holds_alternative<CSSPercentage>(height)) {
      heightValue = std::get<CSSPercentage>(height);
    }

    return CSSXywhShape{.x = xValue, .y = yValue, .width = widthValue, .height = heightValue};
  }
};

static_assert(CSSDataType<CSSXywhShape>);

} // namespace facebook::react
