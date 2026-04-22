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

namespace facebook::react {

using DynamicPropertyId = uint32_t;

struct DynamicPropertiesMap : std::unordered_map<DynamicPropertyId, CSSCalc> {
  using Base = std::unordered_map<DynamicPropertyId, CSSCalc>;
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
