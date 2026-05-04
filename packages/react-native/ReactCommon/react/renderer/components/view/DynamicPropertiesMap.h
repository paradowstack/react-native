/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/view/DynamicResolveContext.h>
#include <react/renderer/components/view/primitives.h>
#include <react/renderer/css/CSSCalc.h>

#include <variant>

namespace facebook::react {

using DynamicPropertyId = uint32_t;

// Typed wrappers for CSSCalc entries stored in DynamicPropertiesMap.
// The variant arm encodes what the calc expression resolves to, replacing
// the old NumericValueDomain field that was stored in NumericValueDynamic.
struct NumberCalcEntry {
  CSSCalc calc;
  bool operator==(const NumberCalcEntry&) const = default;
};
struct LengthCalcEntry {
  CSSCalc calc;
  bool operator==(const LengthCalcEntry&) const = default;
};
struct LengthOrPercentageCalcEntry {
  CSSCalc calc;
  bool operator==(const LengthOrPercentageCalcEntry&) const = default;
};

using TypedCalcEntry =
    std::variant<NumberCalcEntry, LengthCalcEntry, LengthOrPercentageCalcEntry>;

struct DynamicPropertiesMap
    : std::unordered_map<DynamicPropertyId, TypedCalcEntry> {
  using Base = std::unordered_map<DynamicPropertyId, TypedCalcEntry>;
  using Base::Base;

  uint32_t nextId{1};

  uint32_t allocateId() {
    return nextId++;
  }

  bool operator==(const DynamicPropertiesMap& other) const {
    return static_cast<const Base&>(*this) == static_cast<const Base&>(other);
  }
};

const char DynamicPropertiesMapKey[] = "DynamicPropertiesMap";

} // namespace facebook::react
