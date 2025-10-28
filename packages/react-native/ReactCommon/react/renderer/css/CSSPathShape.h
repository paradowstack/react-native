/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <string>

#include <react/renderer/css/CSSDataType.h>
#include <react/renderer/css/CSSValueParser.h>
#include <react/utils/iequals.h>

namespace facebook::react {

/**
 * Representation of CSS path() function
 * path(svg-path-string)
 */
struct CSSPathShape {
  std::string pathData;

  bool operator==(const CSSPathShape &other) const
  {
    return pathData == other.pathData;
  }
};

template <>
struct CSSDataTypeParser<CSSPathShape> {
  static auto consumeFunctionBlock(const CSSFunctionBlock &func, CSSSyntaxParser &parser) -> std::optional<CSSPathShape>
  {
    if (!iequals(func.name, "path")) {
      return {};
    }

    // TODO: String tokens are not yet supported in the CSS tokenizer
    // For now, path() will not parse successfully
    // This needs to be implemented when string token support is added
    return std::nullopt;
  }
};

static_assert(CSSDataType<CSSPathShape>);

} // namespace facebook::react
