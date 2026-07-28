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

  // Families that went dirty (i.e. were part of `affectedLayoutableNodes`)
  // since the last time `runResizeObservations` drained this map. Populated
  // by the commit hook (potentially off the JS thread) and drained by the
  // event-loop step (always on the JS thread).
  //
  // Raw pointers used for identity comparison only, never dereferenced: safe
  // because a family is only ever inserted here while a live observer holds
  // its `ShadowNodeFamily::Shared`, and `unobserve` erases it from this set
  // once no observer targets it anymore.
  std::unordered_map<SurfaceId, std::unordered_set<const ShadowNodeFamily*>>
      dirtyFamiliesBySurfaceId_;

  // Surfaces that have at least one observer awaiting its first delivery
  // check (`ResizeObserver::needsInitialDeliveryCheck`), e.g. just registered
  // via `observe()`, or reset back to needing a check (an `EmptyLayoutMetrics`
  // result). Lets `runResizeObservations` find these surfaces without a full
  // scan of `observersBySurfaceId_` on every tick. Only touched on the JS
  // thread (`observe`, `unobserve`, `runResizeObservations`), so no separate
  // mutex is required; kept in sync as observers are created, removed, and as
  // their initial delivery settles.
  std::unordered_set<SurfaceId> surfaceIdsWithPendingInitialDelivery_;

  // Surfaces that committed since the last `runResizeObservations` and have at
  // least one observer. Used to re-check already-delivered observers for
  // targets that left the tree: removals never appear in
  // `affectedLayoutableNodes`, so they can't be detected via dirty families.
  // Guarded by the same mutex as `dirtyFamiliesBySurfaceId_`.
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
