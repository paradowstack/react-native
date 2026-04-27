/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <iostream>
#include <optional>

#include <react/renderer/components/view/DynamicPropertiesMap.h>
#include <react/renderer/components/view/NumericValueConversions.h>
#include <react/renderer/components/view/primitives.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/core/RawPropsKey.h>
#include <react/renderer/core/graphicsConversions.h>

namespace facebook::react {

#ifdef RN_SERIALIZABLE_STATE

inline folly::dynamic toDynamic(const std::vector<bool>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

inline folly::dynamic toDynamic(const std::vector<std::string>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto& value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

inline folly::dynamic toDynamic(const std::vector<int>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

inline folly::dynamic toDynamic(const std::vector<double>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

inline folly::dynamic toDynamic(const std::vector<Float>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

inline folly::dynamic toDynamic(const std::vector<folly::dynamic>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (auto& value : arrayValue) {
    resultArray.push_back(value);
  }
  return resultArray;
}

template <typename T>
folly::dynamic toDynamic(const std::vector<T>& arrayValue) {
  folly::dynamic resultArray = folly::dynamic::array();
  for (const auto& value : arrayValue) {
    resultArray.push_back(toDynamic(value));
  }
  return resultArray;
}

#endif

/**
 * Use this only when a prop update has definitely been sent from JS;
 * essentially, cases where rawValue is virtually guaranteed to not be a
 * nullptr.
 */
template <typename T>
void fromRawValue(
    const PropsParserContext& context,
    const RawValue& rawValue,
    T& result,
    T defaultValue) {
  if (!rawValue.hasValue()) {
    result = std::move(defaultValue);
    return;
  }

  fromRawValue(context, rawValue, result);
}

template <typename T>
void fromRawValue(
    const PropsParserContext& context,
    const RawValue& rawValue,
    T& result) {
  result = (T)rawValue;
}

template <typename T>
void fromRawValue(
    const PropsParserContext& context,
    const RawValue& rawValue,
    std::optional<T>& result) {
  T resultValue;
  fromRawValue(context, rawValue, resultValue);
  result = std::optional<T>{std::move(resultValue)};
}

template <typename T>
void fromRawValue(
    const PropsParserContext& context,
    const RawValue& rawValue,
    std::vector<T>& result) {
  if (rawValue.hasType<std::vector<RawValue>>()) {
    auto items = (std::vector<RawValue>)rawValue;
    auto length = items.size();
    result.clear();
    result.reserve(length);
    for (auto& item : items) {
      T itemResult;
      fromRawValue(context, item, itemResult);
      result.push_back(itemResult);
    }
    return;
  }

  // The case where `value` is not an array.
  result.clear();
  result.reserve(1);
  T itemResult;
  fromRawValue(context, rawValue, itemResult);
  result.push_back(itemResult);
}

template <typename T>
void fromRawValue(
    const PropsParserContext& context,
    const RawValue& rawValue,
    std::vector<std::vector<T>>& result) {
  if (rawValue.hasType<std::vector<std::vector<RawValue>>>()) {
    auto items = (std::vector<std::vector<RawValue>>)rawValue;
    auto length = items.size();
    result.clear();
    result.reserve(length);
    for (auto& item : items) {
      T itemResult;
      fromRawValue(context, item, itemResult);
      result.push_back(itemResult);
    }
    return;
  }

  // The case where `value` is not an array.
  result.clear();
  result.reserve(1);
  T itemResult;
  fromRawValue(context, rawValue, itemResult);
  result.push_back(itemResult);
}

template <>
inline void fromRawValue(
    const PropsParserContext& /*context*/,
    const RawValue& value,
    Float& result) {
  if (value.hasType<Float>()) {
    result = (Float)value;
    return;
  }
  if (!value.hasType<std::string>()) {
    return;
  }

  auto calc = parseCSSProperty<CSSCalc>((std::string)value);
  if (std::holds_alternative<CSSCalc>(calc)) {
    auto cssCalc = std::get<CSSCalc>(calc);
    if (cssCalc.isUnitless() || cssCalc.isPointsOnly()) {
      result = (Float)cssCalc.px;
    }
  }
}

template <typename T, typename U = T>
T convertRawProp(
    const PropsParserContext& context,
    const RawProps& rawProps,
    const char* name,
    const T& sourceValue,
    const U& defaultValue,
    const char* namePrefix = nullptr,
    const char* nameSuffix = nullptr) {
  const auto* rawValue = rawProps.at(name, namePrefix, nameSuffix);
  if (rawValue == nullptr) [[likely]] {
    return sourceValue;
  }

  // Special case: `null` always means "the prop was removed, use default
  // value".
  if (!rawValue->hasValue()) [[unlikely]] {
    return defaultValue;
  }

  try {
    T result;
    fromRawValue(context, *rawValue, result);
    return result;
  } catch (const std::exception& e) {
    // In case of errors, log the error and fall back to the default
    RawPropsKey key{.prefix = namePrefix, .name = name, .suffix = nameSuffix};
    // TODO: report this using ErrorUtils so it's more visible to the user
    LOG(ERROR) << "Error while converting prop '"
               << static_cast<std::string>(key) << "': " << e.what();
    return defaultValue;
  }
}

inline UntypedNumericValue convertRawProp(
    const PropsParserContext& context,
    const RawProps& rawProps,
    const char* name,
    const UntypedNumericValue& sourceValue,
    const UntypedNumericValue& defaultValue,
    const char* namePrefix = nullptr,
    const char* nameSuffix = nullptr) {
  const auto* rawValue = rawProps.at(name, namePrefix, nameSuffix);
  if (rawValue == nullptr) [[likely]] {
    return sourceValue;
  }

  if (!rawValue->hasValue()) [[unlikely]] {
    return defaultValue;
  }

  try {
    UntypedNumericValue result =
        !sourceValue.isUndefined() ? sourceValue : defaultValue;
    fromRawValue(context, *rawValue, result);
    return result;
  } catch (const std::exception& e) {
    RawPropsKey key{.prefix = namePrefix, .name = name, .suffix = nameSuffix};
    LOG(ERROR) << "Error while converting prop '"
               << static_cast<std::string>(key) << "': " << e.what();
    return defaultValue;
  }
}

} // namespace facebook::react
