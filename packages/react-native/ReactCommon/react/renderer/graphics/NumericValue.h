/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string>
#include <variant>

#include <react/debug/react_native_assert.h>
#include <react/renderer/debug/DebugStringConvertible.h>
#include <react/renderer/debug/flags.h>
#include <react/renderer/graphics/Float.h>

#ifdef RN_SERIALIZABLE_STATE
#include <folly/dynamic.h>
#endif

namespace facebook::react {

using NumericValueDynamicId = uint32_t;

enum class NumericValueKind : uint8_t {
  Undefined,
  Number,
  Length,
  Percentage,
  Dynamic,
};

enum class NumericValueDomain : uint8_t {
  Number,
  Length,
  LengthOrPercentage,
};

struct NumericValueNumber {
  Float value{0.0f};

  constexpr bool operator==(const NumericValueNumber& other) const = default;
};

struct NumericValueLength {
  Float value{0.0f};

  constexpr bool operator==(const NumericValueLength& other) const = default;
};

struct NumericValuePercentage {
  Float value{0.0f};

  constexpr bool operator==(const NumericValuePercentage& other) const =
      default;
};

struct NumericValueDynamic {
  NumericValueDynamicId id{0};
  NumericValueDomain domain{NumericValueDomain::Number};

  constexpr bool operator==(const NumericValueDynamic& other) const = default;
};

struct NumberDomain {
  static constexpr NumericValueDomain value = NumericValueDomain::Number;
};

struct LengthDomain {
  static constexpr NumericValueDomain value = NumericValueDomain::Length;
};

struct LengthPercentageDomain {
  static constexpr NumericValueDomain value =
      NumericValueDomain::LengthOrPercentage;
};

struct AnyNumericValueDomain {};

template <typename DomainT>
concept NumericValueHasStaticDomain = requires {
  { DomainT::value } -> std::convertible_to<NumericValueDomain>;
};

template <typename DomainT>
concept SupportsNumber = std::same_as<DomainT, AnyNumericValueDomain> ||
    std::same_as<DomainT, NumberDomain>;

template <typename DomainT>
concept SupportsLength = std::same_as<DomainT, AnyNumericValueDomain> ||
    std::same_as<DomainT, LengthDomain> ||
    std::same_as<DomainT, LengthPercentageDomain>;

template <typename DomainT>
concept SupportsPercentage = std::same_as<DomainT, AnyNumericValueDomain> ||
    std::same_as<DomainT, LengthPercentageDomain>;

template <typename DomainT = AnyNumericValueDomain>
class NumericValue {
 public:
  template <typename OtherDomainT>
  friend class NumericValue;

  using Storage = std::variant<
      std::monostate,
      NumericValueNumber,
      NumericValueLength,
      NumericValuePercentage,
      NumericValueDynamic>;

  constexpr NumericValue() = default;
  constexpr explicit NumericValue(Float value)
    requires SupportsNumber<DomainT>
      : storage_(NumericValueNumber{value}) {}
  constexpr explicit NumericValue(NumericValueDynamicId value)
    requires SupportsNumber<DomainT>
      : storage_(NumericValueDynamic{value, expectedDomain()}) {}
  constexpr explicit NumericValue(NumericValueNumber value)
    requires SupportsNumber<DomainT>
      : storage_(value) {}
  constexpr explicit NumericValue(NumericValueLength value)
    requires SupportsLength<DomainT>
      : storage_(value) {}
  constexpr explicit NumericValue(NumericValuePercentage value)
    requires SupportsPercentage<DomainT>
      : storage_(value) {}
  constexpr explicit NumericValue(NumericValueDynamic value)
      : storage_(value) {}

  template <typename OtherDomainT>
  constexpr NumericValue(const NumericValue<OtherDomainT>& other)
      : storage_(other.storage_) {}

  template <typename OtherDomainT>
  constexpr NumericValue& operator=(const NumericValue<OtherDomainT>& other) {
    storage_ = other.storage_;
    return *this;
  }

  static constexpr NumericValue number(Float value) {
    static_assert(
        SupportsNumber<DomainT>,
        "NumericValue domain does not support unitless numbers");
    return NumericValue{NumericValueNumber{value}};
  }

  static constexpr NumericValue length(Float value) {
    static_assert(
        SupportsLength<DomainT>,
        "NumericValue domain does not support lengths");
    return NumericValue{NumericValueLength{value}};
  }

  static constexpr NumericValue percentage(Float value) {
    static_assert(
        SupportsPercentage<DomainT>,
        "NumericValue domain does not support percentages");
    return NumericValue{NumericValuePercentage{value}};
  }

  static constexpr NumericValue dynamic(NumericValueDynamicId value) {
    return dynamic(value, expectedDomain());
  }

  static constexpr NumericValue dynamic(
      NumericValueDynamicId value,
      NumericValueDomain domain) {
    if constexpr (NumericValueHasStaticDomain<DomainT>) {
      react_native_assert(domain == DomainT::value);
      return NumericValue{NumericValueDynamic{value, DomainT::value}};
    }
    return NumericValue{NumericValueDynamic{value, domain}};
  }

  constexpr bool operator==(const NumericValue& other) const = default;

  constexpr bool isUndefined() const {
    return std::holds_alternative<std::monostate>(storage_);
  }

  constexpr bool isNumber() const {
    return std::holds_alternative<NumericValueNumber>(storage_);
  }

  constexpr bool isLength() const {
    return std::holds_alternative<NumericValueLength>(storage_);
  }

  constexpr bool isPercentage() const {
    return std::holds_alternative<NumericValuePercentage>(storage_);
  }

  constexpr bool isDynamic() const {
    return std::holds_alternative<NumericValueDynamic>(storage_);
  }

  constexpr NumericValueKind kind() const {
    if (isNumber()) {
      return NumericValueKind::Number;
    }
    if (isLength()) {
      return NumericValueKind::Length;
    }
    if (isPercentage()) {
      return NumericValueKind::Percentage;
    }
    if (isDynamic()) {
      return NumericValueKind::Dynamic;
    }
    return NumericValueKind::Undefined;
  }

  constexpr explicit operator bool() const {
    return !isUndefined();
  }

  static constexpr NumericValueDomain expectedDomain() {
    if constexpr (NumericValueHasStaticDomain<DomainT>) {
      return DomainT::value;
    }

    return NumericValueDomain::LengthOrPercentage;
  }

  static constexpr NumericValueDomain staticDomain()
    requires NumericValueHasStaticDomain<DomainT>
  {
    return DomainT::value;
  }

  constexpr NumericValue<> unwrap() const {
    return NumericValue<>{*this};
  }

  constexpr NumericValueDynamicId asDynamicId() const {
    return isDynamic() ? std::get<NumericValueDynamic>(storage_).id : 0;
  }

  constexpr NumericValueDomain domain() const {
    if (isUndefined()) {
      return expectedDomain();
    }
    if (isNumber()) {
      return NumericValueDomain::Number;
    }
    if (isLength()) {
      return NumericValueDomain::Length;
    }
    if (isPercentage()) {
      return NumericValueDomain::LengthOrPercentage;
    }
    if (isDynamic()) {
      return std::get<NumericValueDynamic>(storage_).domain;
    }
    return NumericValueDomain::LengthOrPercentage;
  }

  constexpr Float asFloat() const {
    if (isNumber()) {
      return std::get<NumericValueNumber>(storage_).value;
    }
    if (isLength()) {
      return std::get<NumericValueLength>(storage_).value;
    }
    if (isPercentage()) {
      return std::get<NumericValuePercentage>(storage_).value;
    }
    return std::numeric_limits<Float>::quiet_NaN();
  }

  constexpr bool isNan() const {
    return isNumber() && std::isnan(asFloat());
  }

  constexpr Float resolve(Float referenceLength = 0.0f) const {
    if (isPercentage()) {
      return std::get<NumericValuePercentage>(storage_).value *
          referenceLength * 0.01f;
    }
    if (isNumber()) {
      return std::get<NumericValueNumber>(storage_).value;
    }
    if (isLength()) {
      return std::get<NumericValueLength>(storage_).value;
    }
    return 0.0f;
  }

  constexpr bool matchesDomain(NumericValueDomain domain) const {
    switch (domain) {
      case NumericValueDomain::Number:
        return isNumber() ||
            (isDynamic() && this->domain() == NumericValueDomain::Number);
      case NumericValueDomain::Length:
        return isLength() ||
            (isDynamic() && this->domain() == NumericValueDomain::Length);
      case NumericValueDomain::LengthOrPercentage:
        return isLength() || isPercentage() ||
            (isDynamic() &&
             this->domain() == NumericValueDomain::LengthOrPercentage);
    }

    return false;
  }

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
  std::string toString() const;
#endif

 private:
  Storage storage_{};
};

using UntypedNumericValue = NumericValue<>;
using NumberValue = NumericValue<NumberDomain>;
using LengthValue = NumericValue<LengthDomain>;
using LengthPercentageValue = NumericValue<LengthPercentageDomain>;

#ifdef RN_SERIALIZABLE_STATE
template <typename DomainT>
inline folly::dynamic NumericValue<DomainT>::toDynamic() const {
  switch (kind()) {
    case NumericValueKind::Undefined:
      return nullptr;
    case NumericValueKind::Number:
    case NumericValueKind::Length:
      return asFloat();
    case NumericValueKind::Percentage:
      return ::facebook::react::toString(asFloat(), '%');
    case NumericValueKind::Dynamic:
      return nullptr;
  }
}
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
template <typename DomainT>
inline std::string NumericValue<DomainT>::toString() const {
  switch (kind()) {
    case NumericValueKind::Undefined:
      return "undefined";
    case NumericValueKind::Number:
      return ::facebook::react::toString(asFloat(), '\0');
    case NumericValueKind::Length:
      return ::facebook::react::toString(asFloat(), '\0') + "px";
    case NumericValueKind::Percentage:
      return ::facebook::react::toString(asFloat(), '%');
    case NumericValueKind::Dynamic:
      return "calc(id=" + std::to_string(asDynamicId()) + ")";
  }
}
#endif

} // namespace facebook::react

namespace std {

template <typename DomainT>
struct hash<facebook::react::NumericValue<DomainT>> {
  size_t operator()(const facebook::react::NumericValue<DomainT>& value) const {
    size_t seed = std::hash<uint8_t>()(static_cast<uint8_t>(value.kind()));
    size_t payload = value.isDynamic()
        ? std::hash<facebook::react::NumericValueDynamicId>()(
              value.asDynamicId())
        : std::hash<facebook::react::Float>()(value.asFloat());
    return seed ^ (payload + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  }
};

} // namespace std
