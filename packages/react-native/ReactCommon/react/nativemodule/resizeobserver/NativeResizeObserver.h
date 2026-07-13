/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#if __has_include("FBReactNativeSpecJSI.h") // CocoaPod headers on Apple
#include "FBReactNativeSpecJSI.h"
#else
#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>
#endif
#include <react/renderer/bridging/bridging.h>
#include <react/renderer/observers/resize/ResizeObserverManager.h>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace facebook::react {

using NativeResizeObserverResizeObserverId = ResizeObserverObserverId;
using RectAsTuple = std::tuple<Float, Float, Float, Float>;
using SizeAsTuple = std::tuple<Float, Float>;

using NativeResizeObserverObserveOptions =
    NativeResizeObserverNativeResizeObserverObserveOptions<
        // resizeObserverId
        NativeResizeObserverResizeObserverId,
        // targetShadowNode
        std::shared_ptr<const ShadowNode>,
        // box
        std::optional<std::string>>;

template <>
struct Bridging<NativeResizeObserverObserveOptions>
    : NativeResizeObserverNativeResizeObserverObserveOptionsBridging<
          NativeResizeObserverObserveOptions> {};

using NativeResizeObserverEntry = NativeResizeObserverNativeResizeObserverEntry<
    // resizeObserverId
    NativeResizeObserverResizeObserverId,
    // targetInstanceHandle
    jsi::Value,
    // contentRect
    RectAsTuple,
    // borderBoxSize
    SizeAsTuple,
    // contentBoxSize
    SizeAsTuple>;

template <>
struct Bridging<NativeResizeObserverEntry>
    : NativeResizeObserverNativeResizeObserverEntryBridging<
          NativeResizeObserverEntry> {};

class NativeResizeObserver
    : public NativeResizeObserverCxxSpec<NativeResizeObserver> {
 public:
  NativeResizeObserver(std::shared_ptr<CallInvoker> jsInvoker);

  void observe(
      jsi::Runtime& runtime,
      NativeResizeObserverObserveOptions options);

  void unobserve(
      jsi::Runtime& runtime,
      NativeResizeObserverResizeObserverId resizeObserverId,
      std::shared_ptr<const ShadowNode> targetShadowNode);

  void connect(
      jsi::Runtime& runtime,
      AsyncCallback<> notifyResizeObserversFunction);

  void disconnect(jsi::Runtime& runtime);

  std::vector<NativeResizeObserverEntry> takeRecords(jsi::Runtime& runtime);

 private:
  ResizeObserverManager resizeObserverManager_{};

  static UIManager& getUIManagerFromRuntime(jsi::Runtime& runtime);
  static ResizeObserverBoxOptions boxOptionsFromOptionalString(
      const std::optional<std::string>& box);
  static NativeResizeObserverEntry convertToNativeModuleEntry(
      const ResizeObserverEntry& entry,
      jsi::Runtime& runtime);
};

} // namespace facebook::react
