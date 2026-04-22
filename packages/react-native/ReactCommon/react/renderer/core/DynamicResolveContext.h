/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/LayoutContext.h>
#include <react/renderer/core/LayoutMetrics.h>

namespace facebook::react {

struct DynamicResolveContext {
  LayoutMetrics layoutMetrics{};
  LayoutContext layoutContext{};
  Float fontSize{0.0f};
  Float rootFontSize{0.0f};

  DynamicResolveContext() = default;
  DynamicResolveContext(
      LayoutMetrics layoutMetrics,
      LayoutContext layoutContext,
      Float fontSize = 0.0f,
      Float rootFontSize = 0.0f)
      : layoutMetrics(layoutMetrics),
        layoutContext(layoutContext),
        fontSize(fontSize),
        rootFontSize(rootFontSize) {}

  Float viewportWidth() const {
    return layoutContext.viewportSize.width;
  }
  Float viewportHeight() const {
    return layoutContext.viewportSize.height;
  }
};

inline bool operator==(const DynamicResolveContext &lhs, const DynamicResolveContext &rhs)
{
  return std::tie(
             lhs.layoutMetrics,
             lhs.layoutContext,
             lhs.fontSize,
             lhs.rootFontSize) ==
      std::tie(
             rhs.layoutMetrics,
             rhs.layoutContext,
             rhs.fontSize,
             rhs.rootFontSize);
}

} // namespace facebook::react
