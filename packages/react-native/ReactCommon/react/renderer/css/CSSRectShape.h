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
 * Representation of CSS rect() function
 * rect(top, right, bottom, left)
 */
struct CSSRectShape {
  std::variant<CSSLength, CSSPercentage> top;
  std::variant<CSSLength, CSSPercentage> right;
  std::variant<CSSLength, CSSPercentage> bottom;
  std::variant<CSSLength, CSSPercentage> left;
  std::optional<std::variant<CSSLength, CSSPercentage>> borderRadius;

  bool operator==(const CSSRectShape &other) const = default;
};

template <>
struct CSSDataTypeParser<CSSRectShape> {
  static auto consumeFunctionBlock(const CSSFunctionBlock &func, CSSSyntaxParser &parser) -> std::optional<CSSRectShape>
  {
    if (!iequals(func.name, "rect")) {
      return {};
    }

    // Parse: top, right, bottom, left (comma-separated)
    auto top = parseNextCSSValue<CSSKeyword, CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(top)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto right = parseNextCSSValue<CSSKeyword, CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(right)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto bottom = parseNextCSSValue<CSSKeyword, CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(bottom)) {
      return std::nullopt;
    }

    parser.consumeWhitespace();

    auto left = parseNextCSSValue<CSSKeyword, CSSLengthPercentage>(parser);
    if (std::holds_alternative<std::monostate>(left)) {
      return std::nullopt;
    }

    // Convert to variant<CSSLength, CSSPercentage>
    std::variant<CSSLength, CSSPercentage> topValue;
    std::variant<CSSLength, CSSPercentage> rightValue;
    std::variant<CSSLength, CSSPercentage> bottomValue;
    std::variant<CSSLength, CSSPercentage> leftValue;

    if (std::holds_alternative<CSSLength>(top)) {
      topValue = std::get<CSSLength>(top);
    } else if (std::holds_alternative<CSSPercentage>(top)) {
      topValue = std::get<CSSPercentage>(top);
    } else if (std::holds_alternative<CSSKeyword>(top)) {
      topValue = CSSPercentage{0.0f};
    }

    if (std::holds_alternative<CSSLength>(right)) {
      rightValue = std::get<CSSLength>(right);
    } else if (std::holds_alternative<CSSPercentage>(right)) {
      rightValue = std::get<CSSPercentage>(right);
    } else if (std::holds_alternative<CSSKeyword>(right)) {
      rightValue = CSSPercentage{100.0f};
    }

    if (std::holds_alternative<CSSLength>(bottom)) {
      bottomValue = std::get<CSSLength>(bottom);
    } else if (std::holds_alternative<CSSPercentage>(bottom)) {
      bottomValue = std::get<CSSPercentage>(bottom);
    } else if (std::holds_alternative<CSSKeyword>(bottom)) {
      bottomValue = CSSPercentage{100.0f};
    }

    if (std::holds_alternative<CSSLength>(left)) {
      leftValue = std::get<CSSLength>(left);
    } else if (std::holds_alternative<CSSPercentage>(left)) {
      leftValue = std::get<CSSPercentage>(left);
    } else if (std::holds_alternative<CSSKeyword>(left)) {
      leftValue = CSSPercentage{0.0f};
    }

    parser.consumeWhitespace();

    auto roundResult = parser.consumeComponentValue<bool>([](const CSSPreservedToken &token) -> bool {
      return token.type() == CSSTokenType::Ident && fnv1aLowercase(token.stringValue()) == fnv1a("round");
    });

    std::optional<std::variant<CSSLength, CSSPercentage>> borderRadius;
    if (roundResult) {
      parser.consumeWhitespace();
      auto radius = parseNextCSSValue<CSSLengthPercentage>(parser);
      if (std::holds_alternative<CSSLength>(radius)) {
        borderRadius = std::get<CSSLength>(radius);
      } else if (std::holds_alternative<CSSPercentage>(radius)) {
        borderRadius = std::get<CSSPercentage>(radius);
      }
    }

    return CSSRectShape{
        .top = topValue, .right = rightValue, .bottom = bottomValue, .left = leftValue, .borderRadius = borderRadius};
  }
};

static_assert(CSSDataType<CSSRectShape>);

} // namespace facebook::react
