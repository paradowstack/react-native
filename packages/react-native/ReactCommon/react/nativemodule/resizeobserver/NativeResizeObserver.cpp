/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "NativeResizeObserver.h"
#include <react/renderer/core/ShadowNode.h>
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

void NativeResizeObserver::observe(
    jsi::Runtime& runtime,
    NativeResizeObserverObserveOptions options) {
  auto resizeObserverId = options.resizeObserverId;
  auto shadowNode = options.targetShadowNode;
  auto shadowNodeFamily = shadowNode->getFamilyShared();
  auto boxOptions = boxOptionsFromOptionalString(options.box);
  auto& uiManager = getUIManagerFromRuntime(runtime);

  resizeObserverManager_.observe(
      resizeObserverId, shadowNodeFamily, boxOptions, uiManager);
}

void NativeResizeObserver::unobserve(
    jsi::Runtime& /*runtime*/,
    NativeResizeObserverResizeObserverId resizeObserverId,
    std::shared_ptr<const ShadowNode> targetShadowNode) {
  auto shadowNodeFamily = targetShadowNode->getFamilyShared();
  resizeObserverManager_.unobserve(resizeObserverId, shadowNodeFamily);
}

void NativeResizeObserver::connect(
    jsi::Runtime& runtime,
    AsyncCallback<> notifyResizeObserversFunction) {
  auto& uiManager = getUIManagerFromRuntime(runtime);
  resizeObserverManager_.connect(
      uiManager, std::move(notifyResizeObserversFunction));
}

void NativeResizeObserver::disconnect(jsi::Runtime& runtime) {
  auto& uiManager = getUIManagerFromRuntime(runtime);
  resizeObserverManager_.disconnect(uiManager);
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
  RectAsTuple contentRect = {
      entry.contentRect.origin.x,
      entry.contentRect.origin.y,
      entry.contentRect.size.width,
      entry.contentRect.size.height};
  SizeAsTuple borderBoxSize = {
      entry.borderBoxSize.width, entry.borderBoxSize.height};
  SizeAsTuple contentBoxSize = {
      entry.contentBoxSize.width, entry.contentBoxSize.height};
  SizeAsTuple devicePixelContentBoxSize = {
      entry.devicePixelContentBoxSize.width,
      entry.devicePixelContentBoxSize.height};
  NativeResizeObserverEntry nativeModuleEntry = {
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
