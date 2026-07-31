/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "NativeResizeObserver.h"
#include <react/renderer/core/ShadowNode.h>
#include <react/renderer/runtimescheduler/RuntimeSchedulerBinding.h>
#include <react/renderer/uimanager/UIManagerBinding.h>
#include <react/renderer/uimanager/primitives.h>

#ifdef RN_DISABLE_OSS_PLUGIN_HEADER
#include "Plugins.h"
#endif

std::shared_ptr<facebook::react::TurboModule>
NativeResizeObserverModuleProvider(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
  return std::make_shared<facebook::react::NativeResizeObserver>(
      std::move(jsInvoker));
}

namespace facebook::react {

NativeResizeObserver::NativeResizeObserver(
    std::shared_ptr<CallInvoker> jsInvoker)
    : NativeResizeObserverCxxSpec(std::move(jsInvoker)) {}

jsi::Object NativeResizeObserver::observe(
    jsi::Runtime& runtime,
    NativeResizeObserverObserveOptions options) {
  auto resizeObserverId = options.resizeObserverId;
  auto shadowNode = options.targetShadowNode;
  auto shadowNodeFamily = shadowNode->getFamilyShared();
  auto boxOptions = boxOptionsFromOptionalString(options.box);
  auto& uiManager = getUIManagerFromRuntime(runtime);

  resizeObserverManager_.observe(
      resizeObserverId, shadowNodeFamily, boxOptions, uiManager);

  return tokenFromShadowNodeFamily(runtime, shadowNodeFamily);
}

void NativeResizeObserver::unobserve(
    jsi::Runtime& runtime,
    NativeResizeObserverResizeObserverId resizeObserverId,
    jsi::Object targetToken) {
  auto shadowNodeFamily =
      shadowNodeFamilyFromToken(runtime, std::move(targetToken));
  resizeObserverManager_.unobserve(resizeObserverId, shadowNodeFamily);
}

void NativeResizeObserver::connect(
    jsi::Runtime& runtime,
    NativeResizeObserverNotifyCallback notifyResizeObserversFunction) {
  auto& uiManager = getUIManagerFromRuntime(runtime);

  // `SyncCallback` is move-only, so share it: the manager copies the callable
  // before invoking it, so that a `disconnect()` from within a callback can't
  // destroy it mid-call. It also already holds the runtime it was created with,
  // hence the unused parameter.
  auto callback = std::make_shared<NativeResizeObserverNotifyCallback>(
      std::move(notifyResizeObserversFunction));

  resizeObserverManager_.connect(
      *RuntimeSchedulerBinding::getBinding(runtime)->getRuntimeScheduler(),
      uiManager,
      [callback](jsi::Runtime& /*runtime*/, bool hasResizeLoopError) {
        (*callback)(hasResizeLoopError);
      });
}

void NativeResizeObserver::disconnect(jsi::Runtime& runtime) {
  auto& uiManager = getUIManagerFromRuntime(runtime);
  resizeObserverManager_.disconnect(
      *RuntimeSchedulerBinding::getBinding(runtime)->getRuntimeScheduler(),
      uiManager);
}

std::vector<NativeResizeObserverEntry> NativeResizeObserver::takeRecords(
    jsi::Runtime& runtime) {
  auto entries = resizeObserverManager_.takeRecords();

  std::vector<NativeResizeObserverEntry> nativeModuleEntries;
  nativeModuleEntries.reserve(entries.size());

  for (const auto& entry : entries) {
    nativeModuleEntries.emplace_back(
        convertToNativeModuleEntry(entry, runtime));
  }

  return nativeModuleEntries;
}

NativeResizeObserverEntry NativeResizeObserver::convertToNativeModuleEntry(
    const ResizeObserverEntry& entry,
    jsi::Runtime& runtime) {
  auto contentRect = RectAsTuple{
      entry.contentRect.origin.x,
      entry.contentRect.origin.y,
      entry.contentRect.size.width,
      entry.contentRect.size.height};
  auto borderBoxSize =
      SizeAsTuple{entry.borderBoxSize.width, entry.borderBoxSize.height};
  auto contentBoxSize =
      SizeAsTuple{entry.contentBoxSize.width, entry.contentBoxSize.height};
  auto devicePixelContentBoxSize = SizeAsTuple{
      entry.devicePixelContentBoxSize.width,
      entry.devicePixelContentBoxSize.height};
  auto nativeModuleEntry = NativeResizeObserverEntry{
      entry.resizeObserverId,
      (*entry.shadowNodeFamily).getInstanceHandle(runtime),
      contentRect,
      borderBoxSize,
      contentBoxSize,
      devicePixelContentBoxSize};

  return nativeModuleEntry;
}

UIManager& NativeResizeObserver::getUIManagerFromRuntime(
    jsi::Runtime& runtime) {
  return UIManagerBinding::getBinding(runtime)->getUIManager();
}

ResizeObserverBoxOptions NativeResizeObserver::boxOptionsFromOptionalString(
    const std::optional<std::string>& box) {
  if (box.has_value()) {
    if (box.value() == "border-box") {
      return ResizeObserverBoxOptions::BorderBox;
    }
    if (box.value() == "device-pixel-content-box") {
      return ResizeObserverBoxOptions::DevicePixelContentBox;
    }
  }

  // "content-box" is the default value.
  return ResizeObserverBoxOptions::ContentBox;
}

} // namespace facebook::react
