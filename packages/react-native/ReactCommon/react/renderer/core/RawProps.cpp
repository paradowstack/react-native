/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RawProps.h"

#include <cxxreact/TraceSection.h>
#include <react/debug/react_native_assert.h>
#include <react/renderer/core/RawPropsKey.h>
#include <react/renderer/core/RawPropsParser.h>

namespace facebook::react {

DynamicPropertyPath::ScopedLevel::ScopedLevel(
    DynamicPropertyPath& path,
    std::string_view node)
    : path_(path) {
  path_.push(node);
}

DynamicPropertyPath::ScopedLevel::~ScopedLevel() {
  path_.pop();
}

DynamicPropertyPath::ScopedLevel::operator DynamicPropertyPath&() noexcept {
  return path_;
}
DynamicPropertyPath::ScopedLevel::operator const DynamicPropertyPath&()
    const noexcept {
  return path_;
}

// Modifiers
void DynamicPropertyPath::push(std::string_view node) {
  nodes_.emplace_back(node);
}

void DynamicPropertyPath::pop() noexcept {
  if (!nodes_.empty()) {
    nodes_.pop_back();
  }
}

[[nodiscard]] DynamicPropertyPath::ScopedLevel DynamicPropertyPath::node(
    std::string_view name) {
  return ScopedLevel{*this, name};
}

// Observers
[[nodiscard]] bool DynamicPropertyPath::empty() const noexcept {
  return nodes_.empty();
}

[[nodiscard]] std::size_t DynamicPropertyPath::depth() const noexcept {
  return nodes_.size();
}

// Returns a C++20 std::span for lightweight, read-only viewing of the path
[[nodiscard]] std::span<const DynamicPropertyPath::Node>
DynamicPropertyPath::view() const noexcept {
  return nodes_;
}

// String representation utility
[[nodiscard]] std::string DynamicPropertyPath::to_string(
    std::string_view separator,
    std::string_view node) const {
  if (nodes_.empty())
    return "";

  std::string result;

  // Pre-calculate and reserve exact memory to avoid reallocations
  std::size_t total_length = (nodes_.size() - 1) * separator.size();
  for (const auto& node : nodes_) {
    total_length += node.size();
  }
  if (!node.empty()) {
    total_length += separator.size() + node.size();
  }
  result.reserve(total_length);

  result += nodes_.front();
  for (std::size_t i = 1; i < nodes_.size(); ++i) {
    result += separator;
    result += nodes_[i];
  }
  if (!node.empty()) {
    result += separator;
    result += node;
  }

  return result;
}

/*
 * Creates an object with given `runtime` and `value`.
 */
RawProps::RawProps(jsi::Runtime& runtime, const jsi::Value& value) noexcept {
  if (value.isNull()) {
    mode_ = Mode::Empty;
    return;
  }

  mode_ = Mode::JSI;
  runtime_ = &runtime;
  value_ = jsi::Value(runtime, value);
}

/*
 * Creates an object with given `folly::dynamic` object.
 * Deprecated. Do not use.
 * We need this temporary, only because we have a callsite that does not have
 * a `jsi::Runtime` behind the data.
 */
RawProps::RawProps(folly::dynamic dynamic) noexcept {
  if (dynamic.isNull()) {
    mode_ = Mode::Empty;
    return;
  }

  mode_ = Mode::Dynamic;
  dynamic_ = std::move(dynamic);
}

RawProps::RawProps(const RawProps& other) noexcept : mode_(other.mode_) {
  if (mode_ == Mode::JSI) {
    runtime_ = other.runtime_;
    value_ = jsi::Value(*runtime_, other.value_);
  } else if (mode_ == Mode::Dynamic) {
    dynamic_ = other.dynamic_;
  }
}

void RawProps::parse(const RawPropsParser& parser) noexcept {
  react_native_assert(parser_ == nullptr && "A parser was already assigned.");
  parser_ = &parser;
  parser.preparse(*this);
}

/*
 * Deprecated. Do not use.
 * The support for explicit conversion to `folly::dynamic` is deprecated and
 * will be removed as soon Android implementation does not need it.
 */
RawProps::operator folly::dynamic() const {
  return toDynamic();
}

/*
 * Deprecated. Do not use.
 * The support for explicit conversion to `folly::dynamic` is deprecated and
 * will be removed as soon Android implementation does not need it.
 */
folly::dynamic RawProps::toDynamic(
    const std::function<bool(const std::string&)>& filterObjectKeys) const {
  switch (mode_) {
    case Mode::Empty:
      return folly::dynamic::object();
    case Mode::JSI: {
      if (filterObjectKeys != nullptr) {
        // We need to filter props
        return jsi::dynamicFromValue(
            *runtime_, value_, [&](const std::string& key) {
              if (filterObjectKeys) {
                return filterObjectKeys(key);
              }
              return false;
            });
      } else {
        // We don't need to filter, just include all props by default
        return jsi::dynamicFromValue(*runtime_, value_, nullptr);
      }
    }
    case Mode::Dynamic:
      return dynamic_;
  }
}

/*
 * Returns `true` if the object is empty.
 * Empty `RawProps` does not have any stored data.
 */
bool RawProps::isEmpty() const noexcept {
  return mode_ == Mode::Empty;
}

/*
 * Returns a const unowning pointer to `RawValue` of a prop with a given name.
 * Returns `nullptr` if a prop with the given name does not exist.
 */
const RawValue* RawProps::at(
    const char* name,
    const char* prefix,
    const char* suffix) const noexcept {
  react_native_assert(
      parser_ &&
      "The object is not parsed. `parse` must be called before `at`.");
  return parser_->at(
      *this, RawPropsKey{.prefix = prefix, .name = name, .suffix = suffix});
}

} // namespace facebook::react
