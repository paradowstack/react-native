/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/debug/flags.h>
#include <string>

#ifdef RN_SERIALIZABLE_STATE
#include <folly/dynamic.h>
#endif

namespace facebook::react {

enum class UnitType {
  Undefined,
  Point,
  Percent,
  CalcId
};

struct ValueUnit {
  float value{0.0f};
  UnitType unit{UnitType::Undefined};

  constexpr ValueUnit() = default;
  constexpr ValueUnit(float v, UnitType u) : value(v), unit(u) {}
  constexpr ValueUnit(uint32_t v) : value(std::bit_cast<float>(v)), unit(UnitType::CalcId) {}

  constexpr bool operator==(const ValueUnit &other) const = default;

  constexpr float resolve(float referenceLength) const
  {
    switch (unit) {
      case UnitType::Point:
        return value;
      case UnitType::Percent:
        return value * referenceLength * 0.01f;
      case UnitType::Undefined:
      case UnitType::CalcId:
      default:
        return 0.0f;
    }
  }

  constexpr operator bool() const
  {
    return unit != UnitType::Undefined;
  }

  constexpr uint32_t calcId() const
  {
    return unit == UnitType::CalcId ? std::bit_cast<uint32_t>(value) : 0;
  }

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
  std::string toString() const;
#endif
};
} // namespace facebook::react
