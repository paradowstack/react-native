#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <numeric>
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
inline constexpr NumericValueDynamicId NumericValueInvalidDynamicId =
    std::numeric_limits<NumericValueDynamicId>::max();

enum class NumericValueKind : uint8_t {
  Undefined,
  Number,
  Length,
  Percentage,
  Dynamic,
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

struct NumericValueLengthPercentage {
  std::variant<NumericValueLength, NumericValuePercentage> value;

  constexpr NumericValueLengthPercentage() : value{NumericValueLength{0.0f}} {}
  constexpr explicit NumericValueLengthPercentage(NumericValueLength l)
      : value{l} {}
  constexpr explicit NumericValueLengthPercentage(NumericValuePercentage p)
      : value{p} {}

  constexpr bool operator==(const NumericValueLengthPercentage& other) const =
      default;
};

struct NumericValueAny {
  std::variant<NumericValueNumber, NumericValueLength, NumericValuePercentage>
      value;

  constexpr NumericValueAny() : value{NumericValueNumber{0.0f}} {}
  constexpr explicit NumericValueAny(NumericValueNumber n) : value{n} {}
  constexpr explicit NumericValueAny(NumericValueLength l) : value{l} {}
  constexpr explicit NumericValueAny(NumericValuePercentage p) : value{p} {}

  constexpr bool operator==(const NumericValueAny& other) const = default;
};

struct NumericValueDynamic {
  NumericValueDynamicId id{0};

  constexpr bool operator==(const NumericValueDynamic& other) const = default;
};
struct NumericValueInvalidDynamic {
  constexpr bool operator==(const NumericValueInvalidDynamic& other) const =
      default;
};

template <typename T>
concept NumericValueConcreteType = std::same_as<T, NumericValueNumber> ||
    std::same_as<T, NumericValueLength> ||
    std::same_as<T, NumericValuePercentage> ||
    std::same_as<T, NumericValueLengthPercentage> ||
    std::same_as<T, NumericValueAny>;

template <typename T>
concept SupportsNumber =
    std::same_as<T, NumericValueNumber> || std::same_as<T, NumericValueAny>;

template <typename T>
concept SupportsLength = std::same_as<T, NumericValueLength> ||
    std::same_as<T, NumericValueLengthPercentage> ||
    std::same_as<T, NumericValueAny>;

template <typename T>
concept SupportsPercentage = std::same_as<T, NumericValuePercentage> ||
    std::same_as<T, NumericValueLengthPercentage> ||
    std::same_as<T, NumericValueAny>;

template <NumericValueConcreteType T = NumericValueAny>
class NumericValue {
 public:
  template <NumericValueConcreteType OtherT>
  friend class NumericValue;

  using Storage = std::variant<
      std::monostate,
      NumericValueDynamic,
      NumericValueInvalidDynamic,
      T>;

  constexpr NumericValue() = default;

  constexpr explicit NumericValue(T concreteValue)
      : storage_(std::move(concreteValue)) {}

  constexpr explicit NumericValue(NumericValueDynamic dynamicValue)
      : storage_(dynamicValue) {}

  constexpr explicit NumericValue(NumericValueInvalidDynamic invalidValue)
      : storage_(invalidValue) {}

  template <typename U>
    requires std::constructible_from<T, U> &&
      (!std::same_as<std::decay_t<U>, NumericValue<T>>) &&
      (!std::same_as<std::decay_t<U>, NumericValueDynamic>)
  constexpr NumericValue(U&& inner) : storage_(T{std::forward<U>(inner)}) {}

  template <NumericValueConcreteType OtherT>
    requires(!std::same_as<T, OtherT>)
  constexpr NumericValue(const NumericValue<OtherT>& other) {
    assignFrom(other);
  }

  template <NumericValueConcreteType OtherT>
    requires(!std::same_as<T, OtherT>)
  constexpr NumericValue& operator=(const NumericValue<OtherT>& other) {
    assignFrom(other);
    return *this;
  }

  constexpr bool operator==(const NumericValue& other) const = default;

  constexpr explicit operator bool() const {
    return !isUndefined();
  }

  constexpr bool is_undefined() const {
    return std::holds_alternative<std::monostate>(storage_);
  }

  constexpr bool is_resolved() const {
    return std::holds_alternative<T>(storage_);
  }

  constexpr bool is_dynamic() const {
    return std::holds_alternative<NumericValueDynamic>(storage_);
  }

  constexpr bool isUndefined() const {
    return is_undefined();
  }

  constexpr bool isDynamic() const {
    return is_dynamic();
  }

  constexpr bool isInvalid() const {
    return is_dynamic() && asDynamicId() == NumericValueInvalidDynamicId;
  }

  constexpr bool isNumber() const
    requires SupportsNumber<T>
  {
    if (!is_resolved()) {
      return false;
    }
    if constexpr (std::same_as<T, NumericValueNumber>) {
      return true;
    } else {
      return std::holds_alternative<NumericValueNumber>(
          std::get<T>(storage_).value);
    }
  }

  constexpr bool isLength() const
    requires SupportsLength<T>
  {
    if (!is_resolved()) {
      return false;
    }
    if constexpr (std::same_as<T, NumericValueLength>) {
      return true;
    } else {
      return std::holds_alternative<NumericValueLength>(
          std::get<T>(storage_).value);
    }
  }

  constexpr bool isPercentage() const
    requires SupportsPercentage<T>
  {
    if (!is_resolved()) {
      return false;
    }
    if constexpr (std::same_as<T, NumericValuePercentage>) {
      return true;
    } else {
      return std::holds_alternative<NumericValuePercentage>(
          std::get<T>(storage_).value);
    }
  }

  constexpr Float asNumber() const
    requires SupportsNumber<T>
  {
    react_native_assert(isNumber());
    if constexpr (std::same_as<T, NumericValueNumber>) {
      return std::get<T>(storage_).value;
    } else {
      return std::get<NumericValueNumber>(std::get<T>(storage_).value).value;
    }
  }

  constexpr Float asLength() const
    requires SupportsLength<T>
  {
    react_native_assert(isLength());
    if constexpr (std::same_as<T, NumericValueLength>) {
      return std::get<T>(storage_).value;
    } else {
      return std::get<NumericValueLength>(std::get<T>(storage_).value).value;
    }
  }

  constexpr Float asPercentage() const
    requires SupportsPercentage<T>
  {
    react_native_assert(isPercentage());
    if constexpr (std::same_as<T, NumericValuePercentage>) {
      return std::get<T>(storage_).value;
    } else {
      return std::get<NumericValuePercentage>(std::get<T>(storage_).value)
          .value;
    }
  }

  constexpr Float value() const {
    if (!is_resolved()) {
      return 0.0f;
    }
    return resolvedFloat();
  }

  constexpr Float asFloat() const {
    if (!is_resolved()) {
      return std::numeric_limits<Float>::quiet_NaN();
    }
    return resolvedFloat();
  }

  constexpr bool isNan() const {
    return is_resolved() && std::isnan(resolvedFloat());
  }

  constexpr NumericValueDynamic dynamic_id() const {
    react_native_assert(is_dynamic());
    return std::get<NumericValueDynamic>(storage_);
  }

  constexpr NumericValueDynamicId asDynamicId() const {
    return is_dynamic() ? std::get<NumericValueDynamic>(storage_).id : 0;
  }

  static constexpr NumericValue number(Float v)
    requires SupportsNumber<T>
  {
    if constexpr (std::same_as<T, NumericValueNumber>) {
      return NumericValue{NumericValueNumber{v}};
    } else {
      return NumericValue{T{NumericValueNumber{v}}};
    }
  }

  static constexpr NumericValue length(Float v)
    requires SupportsLength<T>
  {
    if constexpr (std::same_as<T, NumericValueLength>) {
      return NumericValue{NumericValueLength{v}};
    } else {
      return NumericValue{T{NumericValueLength{v}}};
    }
  }

  static constexpr NumericValue percentage(Float v)
    requires SupportsPercentage<T>
  {
    if constexpr (std::same_as<T, NumericValuePercentage>) {
      return NumericValue{NumericValuePercentage{v}};
    } else {
      return NumericValue{T{NumericValuePercentage{v}}};
    }
  }

  static constexpr NumericValue dynamic(NumericValueDynamicId id) {
    return NumericValue{NumericValueDynamic{id}};
  }

  static constexpr NumericValue invalid() {
    return NumericValue{NumericValueDynamic{NumericValueInvalidDynamicId}};
  }

  constexpr NumericValueKind kind() const {
    if (is_undefined()) {
      return NumericValueKind::Undefined;
    }
    if (is_dynamic()) {
      return NumericValueKind::Dynamic;
    }
    if constexpr (std::same_as<T, NumericValueNumber>) {
      return NumericValueKind::Number;
    } else if constexpr (std::same_as<T, NumericValueLength>) {
      return NumericValueKind::Length;
    } else if constexpr (std::same_as<T, NumericValuePercentage>) {
      return NumericValueKind::Percentage;
    } else {
      return std::visit(
          [](const auto& inner) constexpr {
            using I = std::decay_t<decltype(inner)>;
            if constexpr (std::same_as<I, NumericValueNumber>) {
              return NumericValueKind::Number;
            } else if constexpr (std::same_as<I, NumericValueLength>) {
              return NumericValueKind::Length;
            } else {
              return NumericValueKind::Percentage;
            }
          },
          std::get<T>(storage_).value);
    }
  }

  constexpr Float resolve(Float referenceLength = 0.0f) const {
    if constexpr (SupportsPercentage<T>) {
      if (isPercentage()) {
        return asPercentage() * referenceLength * 0.01f;
      }
    }
    if constexpr (SupportsNumber<T>) {
      if (isNumber()) {
        return asNumber();
      }
    }
    if constexpr (SupportsLength<T>) {
      if (isLength()) {
        return asLength();
      }
    }
    return 0.0f;
  }

  constexpr NumericValue<NumericValueAny> unwrap() const {
    return NumericValue<NumericValueAny>{*this};
  }

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const;
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
  std::string toString() const;
#endif

 private:
  Storage storage_{};

  constexpr Float resolvedFloat() const {
    if constexpr (
        std::same_as<T, NumericValueNumber> ||
        std::same_as<T, NumericValueLength> ||
        std::same_as<T, NumericValuePercentage>) {
      return std::get<T>(storage_).value;
    } else {
      return std::visit(
          [](const auto& inner) constexpr { return inner.value; },
          std::get<T>(storage_).value);
    }
  }

  template <NumericValueConcreteType OtherT>
  constexpr void assignFrom(const NumericValue<OtherT>& other) {
    if (other.is_undefined()) {
      storage_ = std::monostate{};
    } else if (other.is_dynamic()) {
      storage_ = other.dynamic_id();
    } else {
      convertResolvedFrom(other);
    }
  }

  template <NumericValueConcreteType OtherT>
  constexpr void convertResolvedFrom(const NumericValue<OtherT>& other) {
    if constexpr (std::same_as<T, NumericValueAny>) {
      if constexpr (
          std::same_as<OtherT, NumericValueNumber> ||
          std::same_as<OtherT, NumericValueLength> ||
          std::same_as<OtherT, NumericValuePercentage>) {
        storage_ = T{std::get<OtherT>(other.storage_)};
      } else if constexpr (std::same_as<OtherT, NumericValueLengthPercentage>) {
        std::visit(
            [this](const auto& inner) { storage_ = T{inner}; },
            std::get<OtherT>(other.storage_).value);
      }
    } else if constexpr (std::same_as<OtherT, NumericValueAny>) {
      std::visit(
          [this](const auto& inner) {
            using I = std::decay_t<decltype(inner)>;
            if constexpr (std::same_as<T, I>) {
              storage_ = inner;
            } else if constexpr (
                std::same_as<T, NumericValueLengthPercentage>) {
              if constexpr (
                  std::same_as<I, NumericValueLength> ||
                  std::same_as<I, NumericValuePercentage>) {
                storage_ = T{inner};
              }
            }
          },
          std::get<OtherT>(other.storage_).value);
    } else if constexpr (
        std::same_as<T, NumericValueLengthPercentage> &&
        (std::same_as<OtherT, NumericValueLength> ||
         std::same_as<OtherT, NumericValuePercentage>)) {
      storage_ = T{std::get<OtherT>(other.storage_)};
    } else if constexpr (
        (std::same_as<T, NumericValueLength> ||
         std::same_as<T, NumericValuePercentage>) &&
        std::same_as<OtherT, NumericValueLengthPercentage>) {
      auto* v = std::get_if<T>(&std::get<OtherT>(other.storage_).value);
      if (v != nullptr) {
        storage_ = *v;
      }
    }
  }
};

using NumberValue = NumericValue<NumericValueNumber>;
using LengthValue = NumericValue<NumericValueLength>;
using PercentageValue = NumericValue<NumericValuePercentage>;
using LengthPercentageValue = NumericValue<NumericValueLengthPercentage>;
using UntypedNumericValue = NumericValue<NumericValueAny>;

#ifdef RN_SERIALIZABLE_STATE
template <NumericValueConcreteType T>
inline folly::dynamic NumericValue<T>::toDynamic() const {
  switch (kind()) {
    case NumericValueKind::Undefined:
      return nullptr;
    case NumericValueKind::Number:
    case NumericValueKind::Length:
      return static_cast<double>(asFloat());
    case NumericValueKind::Percentage:
      return std::to_string(asFloat()) + "%";
    case NumericValueKind::Dynamic:
      return nullptr;
  }
  return nullptr;
}
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
template <NumericValueConcreteType T>
inline std::string NumericValue<T>::toString() const {
  switch (kind()) {
    case NumericValueKind::Undefined:
      return "undefined";
    case NumericValueKind::Number:
      return std::to_string(asFloat());
    case NumericValueKind::Length:
      return std::to_string(asFloat()) + "px";
    case NumericValueKind::Percentage:
      return std::to_string(asFloat()) + "%";
    case NumericValueKind::Dynamic:
      return "undefined";
  }
  return "";
}
#endif

} // namespace facebook::react

namespace std {

template <facebook::react::NumericValueConcreteType T>
struct hash<facebook::react::NumericValue<T>> {
  size_t operator()(const facebook::react::NumericValue<T>& value) const {
    size_t seed = std::hash<uint8_t>()(static_cast<uint8_t>(value.kind()));
    size_t payload = value.isDynamic()
        ? std::hash<facebook::react::NumericValueDynamicId>()(
              value.asDynamicId())
        : std::hash<facebook::react::Float>()(value.asFloat());
    return seed ^ (payload + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  }
};

} // namespace std
