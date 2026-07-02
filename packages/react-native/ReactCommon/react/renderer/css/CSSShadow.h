/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <tuple>

#include <react/renderer/css/CSSCalc.h>
#include <react/renderer/css/CSSColor.h>
#include <react/renderer/css/CSSDataType.h>
#include <react/renderer/css/CSSKeyword.h>
#include <react/renderer/css/CSSLength.h>
#include <react/renderer/css/CSSList.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/utils/to_underlying.h>

namespace facebook::react {

/**
 * Representation of CSS <shadow> data type
 * https://drafts.csswg.org/css-backgrounds/#typedef-shadow
 *
 * Length fields are stored as CSSCalc (a superset of CSSLength) so that
 * viewport-relative values like calc(10vw) are preserved for later resolution.
 */
struct CSSShadow {
  CSSCalc offsetX{};
  CSSCalc offsetY{};
  CSSCalc blurRadius{};
  CSSCalc spreadDistance{};
  CSSColor color{CSSColor::black()};
  bool inset{false};

  constexpr bool operator==(const CSSShadow& rhs) const = default;
};

/**
 * Represents a keyword for an inset shadow.
 */
enum class CSSInsetShadowKeyword : std::underlying_type_t<CSSKeyword> {
  Inset = to_underlying(CSSKeyword::Inset),
};

static_assert(CSSDataType<CSSInsetShadowKeyword>);

template <>
struct CSSDataTypeParser<CSSShadow> {
  static constexpr auto consume(CSSValueParser& parser)
      -> std::optional<CSSShadow> {
    std::optional<CSSColor> color{};
    bool inset{false};
    std::optional<std::tuple<CSSCalc, CSSCalc, CSSCalc, CSSCalc>> lengths{};

    for (auto nextValue = parser.parseNextValue<
                          CSSLength,
                          CSSColor,
                          CSSInsetShadowKeyword,
                          CSSCalc>();
         !std::holds_alternative<std::monostate>(nextValue);
         nextValue = parser.parseNextValue<
                     CSSLength,
                     CSSColor,
                     CSSInsetShadowKeyword,
                     CSSCalc>(CSSDelimiter::Whitespace)) {
      if (std::holds_alternative<CSSLength>(nextValue)) {
        if (lengths.has_value()) {
          return {};
        }
        auto calc = CSSCalc::fromLength(
            std::get<CSSLength>(nextValue).value,
            std::get<CSSLength>(nextValue).unit);
        if (!calc) {
          return {}; // unsupported unit (em, rem, etc.)
        }
        lengths = parseRestCalcs(*calc, parser);
        if (!lengths.has_value()) {
          return {};
        }
        continue;
      }

      if (std::holds_alternative<CSSCalc>(nextValue)) {
        if (lengths.has_value()) {
          return {};
        }
        lengths = parseRestCalcs(std::get<CSSCalc>(nextValue), parser);
        if (!lengths.has_value()) {
          return {};
        }
        continue;
      }

      if (std::holds_alternative<CSSColor>(nextValue)) {
        if (color.has_value()) {
          return {};
        }
        color = std::get<CSSColor>(nextValue);
        continue;
      }

      if (std::holds_alternative<CSSInsetShadowKeyword>(nextValue)) {
        if (inset) {
          return {};
        }
        inset = true;
        continue;
      }
    }

    if (!lengths.has_value()) {
      return {};
    }

    return CSSShadow{
        .offsetX = std::get<0>(*lengths),
        .offsetY = std::get<1>(*lengths),
        .blurRadius = std::get<2>(*lengths),
        .spreadDistance = std::get<3>(*lengths),
        .color = color.value_or(CSSColor::black()),
        .inset = inset,
    };
  }

 private:
  // Parse a single length-or-calc token as CSSCalc.
  static constexpr auto parseNextCalc(CSSValueParser& parser)
      -> std::optional<CSSCalc> {
    auto next =
        parser.parseNextValue<CSSLength, CSSCalc>(CSSDelimiter::Whitespace);
    if (std::holds_alternative<CSSLength>(next)) {
      return CSSCalc::fromLength(
          std::get<CSSLength>(next).value, std::get<CSSLength>(next).unit);
    }
    if (std::holds_alternative<CSSCalc>(next)) {
      return std::get<CSSCalc>(next);
    }
    return std::nullopt;
  }

  // Parse the remaining 1–3 length-or-calc tokens after offsetX has been read.
  static constexpr auto parseRestCalcs(CSSCalc offsetX, CSSValueParser& parser)
      -> std::optional<std::tuple<CSSCalc, CSSCalc, CSSCalc, CSSCalc>> {
    // offsetY is required
    auto offsetY = parseNextCalc(parser);
    if (!offsetY) {
      return {};
    }

    // blurRadius is optional
    auto blurRadius = parseNextCalc(parser);
    if (!blurRadius) {
      return std::make_tuple(offsetX, *offsetY, CSSCalc{}, CSSCalc{});
    }
    // CSS spec: blurRadius must not be negative (only checkable for px-only)
    if (blurRadius->isPointsOnly() && blurRadius->px < 0) {
      return {};
    }

    // spreadDistance is optional
    auto spreadDistance = parseNextCalc(parser);
    if (!spreadDistance) {
      return std::make_tuple(offsetX, *offsetY, *blurRadius, CSSCalc{});
    }

    return std::make_tuple(offsetX, *offsetY, *blurRadius, *spreadDistance);
  }
};

static_assert(CSSDataType<CSSShadow>);

/**
 * Represents a comma separated list of at least one <shadow>
 * <shadow>#
 */
using CSSShadowList = CSSCommaSeparatedList<CSSShadow>;

} // namespace facebook::react
