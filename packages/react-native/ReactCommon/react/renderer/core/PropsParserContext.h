/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>

#include <react/renderer/core/ReactPrimitives.h>
#include <react/utils/ContextContainer.h>

namespace facebook::react {

// Forward declaration to avoid circular dependency (view → core → view).
// DynamicPropertiesMap is defined in
// react/renderer/components/view/DynamicPropertiesMap.h.
struct DynamicPropertiesMap;

// For props requiring some context to parse, this toolbox can be used.
// It should be used as infrequently as possible - most props can and should
// be parsed without any context.
struct PropsParserContext {
  PropsParserContext(
      const SurfaceId surfaceId,
      const ContextContainer& contextContainer)
      : surfaceId(surfaceId), contextContainer(contextContainer) {}

  // Non-copyable
  PropsParserContext(const PropsParserContext&) = delete;
  PropsParserContext& operator=(const PropsParserContext&) = delete;

  const SurfaceId surfaceId;
  const ContextContainer& contextContainer;

  // Non-owning pointer to the DynamicPropertiesMap of the Props object
  // currently being constructed on this call-stack. Set by Props::initialize()
  // and read by fromRawValueWithCalc / parseNumericValue helpers.
  //
  // Using a direct pointer instead of the shared ContextContainer avoids a
  // data race: the ContextContainer is the ComponentDescriptor's shared
  // instance (one per component type), so concurrent Props construction on
  // different threads would overwrite each other's entry and allow one thread
  // to write calc entries into another thread's map while it is being read.
  //
  // Marked mutable so it can be set even when the context is held by const&.
  mutable DynamicPropertiesMap* calcMap{nullptr};
};

} // namespace facebook::react
