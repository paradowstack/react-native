/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cmath>
#include <concepts>

#include <react/renderer/graphics/NumericValue.h>

namespace facebook::react {

constexpr float kDefaultEpsilon = 0.005f;

template <std::floating_point T>
inline bool
floatEquality(T a, T b, T epsilon = static_cast<T>(kDefaultEpsilon)) {
  return (std::isnan(a) && std::isnan(b)) ||
      (!std::isnan(a) && !std::isnan(b) && std::abs(a - b) < epsilon);
}

template <typename LeftDomainT, typename RightDomainT>
inline bool floatEquality(
    const NumericValue<LeftDomainT>& a,
    const NumericValue<RightDomainT>& b,
    Float epsilon = static_cast<Float>(kDefaultEpsilon)) {
  if (a.kind() != b.kind()) {
    return false;
  }

  if (a.isDynamic()) {
    return a.asDynamicId() == b.asDynamicId() && a.domain() == b.domain();
  }

  if (a.isUndefined()) {
    return true;
  }

  return floatEquality(a.asFloat(), b.asFloat(), epsilon);
}

} // namespace facebook::react
