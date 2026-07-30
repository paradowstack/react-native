/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/ShadowNodeFamily.h>
#include <react/renderer/mounting/ShadowTree.h>
#include <react/renderer/mounting/ShadowTreeRegistry.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>
#include <react/renderer/runtimescheduler/RuntimeSchedulerResizeObserverDelegate.h>
#include <react/renderer/uimanager/UIManager.h>
#include <react/renderer/uimanager/UIManagerCommitHook.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ResizeObserver.h"

namespace facebook::react {

class ResizeObserverManager final
    : public UIManagerCommitHook,
      public RuntimeSchedulerResizeObserverDelegate {
 public:
  ResizeObserverManager();

  void observe(
      ResizeObserverObserverId resizeObserverId,
      const ShadowNodeFamily::Shared& shadowNodeFamily,
      ResizeObserverBoxOptions boxOptions,
      UIManager& uiManager);

  void unobserve(
      ResizeObserverObserverId resizeObserverId,
      const ShadowNodeFamily::Shared& shadowNodeFamily);

  void connect(
      RuntimeScheduler& runtimeScheduler,
      UIManager& uiManager,
      std::function<void()> notifyResizeObserversFunction);

  void disconnect(RuntimeScheduler& runtimeScheduler, UIManager& uiManager);

  std::vector<ResizeObserverEntry> takeRecords();

#pragma mark - RuntimeSchedulerResizeObserverDelegate

  void runResizeObservations() override;

#pragma mark - UIManagerCommitHook

  void commitHookWasRegistered(const UIManager& uiManager) noexcept override;
  void commitHookWasUnregistered(const UIManager& uiManager) noexcept override;

  void shadowTreeDidCommit(
      const ShadowTree& shadowTree,
      const RootShadowNode::Shared& rootShadowNode,
      const std::vector<const LayoutableShadowNode*>&
          affectedLayoutableNodes) noexcept override;

 private:
  mutable std::
      unordered_map<SurfaceId, std::vector<std::unique_ptr<ResizeObserver>>>
          observersBySurfaceId_;
  mutable std::mutex observersMutex_;

  // Monotonic counter assigned at `observe()` so deliveries can preserve
  // observation registration order.
  uint64_t nextObservationSequence_{0};

  // Families that went dirty since `runResizeObservations` last drained this
  // map. Written by the commit hook (any thread), drained on the JS thread.
  // Raw pointers, compared by identity only (never dereferenced): a family is
  // only present while a live observer holds its `ShadowNodeFamily::Shared`,
  // and `unobserve` erases it once no observer targets it.
  std::unordered_map<SurfaceId, std::unordered_set<const ShadowNodeFamily*>>
      dirtyFamiliesBySurfaceId_;

  // Surfaces with an observer awaiting its first delivery (just observed, or
  // reset because it has no reportable size). Lets `runResizeObservations`
  // find them without scanning all observers each tick. JS-thread-only, so no
  // mutex.
  std::unordered_set<SurfaceId> surfaceIdsWithPendingInitialDelivery_;

  // Surfaces that committed since the last `runResizeObservations` and have an
  // observer. Used to detect targets that left the tree (removals don't appear
  // in `affectedLayoutableNodes`). Guarded by `dirtyFamiliesMutex_`.
  std::unordered_set<SurfaceId> committedSurfaceIds_;
  std::mutex dirtyFamiliesMutex_;

  std::function<void()> notifyResizeObserversFunction_;
  bool commitHookRegistered_{};

  // This is only accessed from the JS thread at the end of the event loop
  // tick, so it is safe to retain it as a raw pointer.
  // We need to retain it here because the RuntimeScheduler does not provide
  // it when calling `runResizeObservations`.
  const ShadowTreeRegistry* shadowTreeRegistry_{nullptr};

  mutable std::vector<ResizeObserverEntry> pendingEntries_;
  mutable std::mutex pendingEntriesMutex_;
  mutable bool notifiedResizeObservers_{};

  void notifyObserversIfNecessary();
  void notifyObservers();
};

} // namespace facebook::react
