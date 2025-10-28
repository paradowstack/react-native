/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/graphics/ClipPath.h>
#include <optional>
#include <string>

namespace facebook::react {

void parseProcessedClipPath(const PropsParserContext &context, const RawValue &value, std::optional<ClipPath> &result);

void parseUnprocessedClipPathString(std::string &&value, std::optional<ClipPath> &result);

std::optional<ClipPath> parseClipPathRawValue(const PropsParserContext &context, const RawValue &value);

void parseUnprocessedClipPathList(
    const PropsParserContext &context,
    std::vector<RawValue> &&value,
    std::optional<ClipPath> &result);

inline void
parseUnprocessedClipPath(const PropsParserContext &context, const RawValue &value, std::optional<ClipPath> &result)
{
  if (value.hasType<std::string>()) {
    parseUnprocessedClipPathString((std::string)value, result);
  } else if (value.hasType<std::vector<RawValue>>()) {
    parseUnprocessedClipPathList(context, (std::vector<RawValue>)value, result);
  } else {
    result = {};
  }
}

inline void fromRawValue(const PropsParserContext &context, const RawValue &value, std::optional<ClipPath> &result)
{
  if (ReactNativeFeatureFlags::enableNativeCSSParsing()) {
    parseUnprocessedClipPath(context, value, result);
  } else {
    parseProcessedClipPath(context, value, result);
  }
}

} // namespace facebook::react
