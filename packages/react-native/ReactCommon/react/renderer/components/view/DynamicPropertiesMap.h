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
#include <glog/logging.h>

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
//			
//			// 1. Konstruktor uniwersalny (perfect forwarding)
//				template<typename... Args>
//				DynamicPropertiesMap(Args&&... args) : Base(std::forward<Args>(args)...) {
//					LOG(ERROR) << "[DynamicPropertiesMap] CREATED at: " << this;
//				}
//
//				// 2. Konstruktor kopiujący
//				DynamicPropertiesMap(const DynamicPropertiesMap& other)
//					: Base(other), nextId(other.nextId) {
//					LOG(ERROR) << "[DynamicPropertiesMap] COPIED to: " << this << " from: " << &other;
//				}
//
//				// 3. Konstruktor przenoszący
//				DynamicPropertiesMap(DynamicPropertiesMap&& other) noexcept
//					: Base(std::move(other)), nextId(other.nextId) {
//					LOG(ERROR) << "[DynamicPropertiesMap] MOVED to: " << this << " from: " << &other;
//				}
//
//				// 4. Operator przypisania kopiującego (NOWE)
//				DynamicPropertiesMap& operator=(const DynamicPropertiesMap& other) {
//					LOG(ERROR) << "[DynamicPropertiesMap] COPY ASSIGNED to: " << this << " from: " << &other;
//					if (this != &other) {
//						Base::operator=(other);
//						nextId = other.nextId;
//					}
//					return *this;
//				}
//
//				// 5. Operator przypisania przenoszącego (NOWE)
//				DynamicPropertiesMap& operator=(DynamicPropertiesMap&& other) noexcept {
//					LOG(ERROR) << "[DynamicPropertiesMap] MOVE ASSIGNED to: " << this << " from: " << &other;
//					if (this != &other) {
//						Base::operator=(std::move(other));
//						nextId = other.nextId;
//					}
//					return *this;
//				}
//
//				// 6. Destruktor
//				~DynamicPropertiesMap() {
//					LOG(ERROR) << "[DynamicPropertiesMap] DESTROYED at: " << this;
//				}
			
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
