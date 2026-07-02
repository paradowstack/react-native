/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/graphics/NumericValue.h>
#include <optional>

namespace facebook::react {

struct BackgroundPosition {
  std::optional<UntypedNumericValue> top;
  std::optional<UntypedNumericValue> left;
  std::optional<UntypedNumericValue> right;
  std::optional<UntypedNumericValue> bottom;

  BackgroundPosition()
      : top(UntypedNumericValue::length(0.0f)),
        left(UntypedNumericValue::length(0.0f)) {}

  bool operator==(const BackgroundPosition& other) const = default;
};

} // namespace facebook::react
