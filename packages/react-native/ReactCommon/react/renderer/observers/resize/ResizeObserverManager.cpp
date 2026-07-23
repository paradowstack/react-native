/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ResizeObserverManager.h"
#include <cxxreact/TraceSection.h>
#include <react/renderer/core/LayoutableShadowNode.h>
#include <algorithm>
#include <unordered_set>
#include <utility>
#include "ResizeObserver.h"

namespace facebook::react {

namespace {
RootShadowNode::Shared getRootShadowNode(
    SurfaceId surfaceId,
    const ShadowTreeRegistry& shadowTreeRegistry) {
  RootShadowNode::Shared rootShadowNode = nullptr;

  shadowTreeRegistry.visit(surfaceId, [&](const ShadowTree& shadowTree) {
    rootShadowNode = shadowTree.getCurrentRevision().rootShadowNode;
  });

  return rootShadowNode;
}
} // namespace

ResizeObserverManager::ResizeObserverManager() = default;

void ResizeObserverManager::observe(
    ResizeObserverObserverId resizeObserverId,
    const ShadowNodeFamily::Shared& shadowNodeFamily,
    ResizeObserverBoxOptions boxOptions,
    UIManager& /*uiManager*/) {
  TraceSection s("ResizeObserverManager::observe");

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  // Per spec, newly observed targets are only processed on the next "update
  // the rendering" step (i.e. the `runResizeObservations` event-loop step
  // below), not synchronously here. Since `observe` always runs as part of a
  // JS task, and every task is followed by an "update the rendering" step
  // regardless of whether there are pending rendering (mounting) updates,
  // the initial observation is still guaranteed to be delivered promptly, at
  // the end of the current tick.
  std::unique_lock lock(observersMutex_);

  auto& observers = observersBySurfaceId_[surfaceId];

  observers.emplace_back(std::make_unique<ResizeObserver>(
      resizeObserverId, shadowNodeFamily, boxOptions));
}

void ResizeObserverManager::unobserve(
    ResizeObserverObserverId resizeObserverId,
    const ShadowNodeFamily::Shared& shadowNodeFamily) {
  TraceSection s("ResizeObserverManager::unobserve");

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  // Whether another observer still targets the same family after this
  // removal. If so, we must not drop the family's pending dirty state.
  bool familyStillObserved = false;
  {
    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      return;
    }

    auto& observers = observersIt->second;

    observers.erase(
        std::remove_if(
            observers.begin(),
            observers.end(),
            [resizeObserverId, &shadowNodeFamily](const auto& observer) {
              return observer->getResizeObserverId() == resizeObserverId &&
                  observer->getTargetShadowNodeFamily() == shadowNodeFamily;
            }),
        observers.end());

    for (const auto& observer : observers) {
      if (observer->getTargetShadowNodeFamily() == shadowNodeFamily) {
        familyStillObserved = true;
        break;
      }
    }

    if (observers.empty()) {
      observersBySurfaceId_.erase(surfaceId);
    }
  }

  // Drop bookkeeping for the removed observation so we don't run a wasted
  // observation pass for it, deliver a stale entry after `unobserve`, or
  // retain a dangling pointer to its (now potentially freed) family. Only
  // clear the dirty family if no remaining observer still targets it.
  if (!familyStillObserved) {
    std::unique_lock lock(dirtyFamiliesMutex_);
    auto dirtyFamiliesIt = dirtyFamiliesBySurfaceId_.find(surfaceId);
    if (dirtyFamiliesIt != dirtyFamiliesBySurfaceId_.end()) {
      dirtyFamiliesIt->second.erase(shadowNodeFamily.get());
      if (dirtyFamiliesIt->second.empty()) {
        dirtyFamiliesBySurfaceId_.erase(dirtyFamiliesIt);
      }
    }
  }

  {
    std::unique_lock lock(pendingEntriesMutex_);
    pendingEntries_.erase(
        std::remove_if(
            pendingEntries_.begin(),
            pendingEntries_.end(),
            [resizeObserverId, &shadowNodeFamily](const ResizeObserverEntry&
                                                      entry) {
              return entry.resizeObserverId == resizeObserverId &&
                  entry.shadowNodeFamily == shadowNodeFamily;
            }),
        pendingEntries_.end());
  }
}

void ResizeObserverManager::connect(
    RuntimeScheduler& runtimeScheduler,
    UIManager& uiManager,
    std::function<void()> notifyResizeObserversFunction) {
  TraceSection s("ResizeObserverManager::connect");

  // Fail-safe in case the caller doesn't guarantee consistency.
  if (commitHookRegistered_) {
    return;
  }

  notifyResizeObserversFunction_ = std::move(notifyResizeObserversFunction);

  runtimeScheduler.setResizeObserverDelegate(this);
  uiManager.registerCommitHook(*this);
  shadowTreeRegistry_ = &uiManager.getShadowTreeRegistry();
  commitHookRegistered_ = true;
}

void ResizeObserverManager::disconnect(
    RuntimeScheduler& runtimeScheduler,
    UIManager& uiManager) {
  TraceSection s("ResizeObserverManager::disconnect");

  // Fail-safe in case the caller doesn't guarantee consistency.
  if (!commitHookRegistered_) {
    return;
  }

  runtimeScheduler.setResizeObserverDelegate(nullptr);
  uiManager.unregisterCommitHook(*this);
  shadowTreeRegistry_ = nullptr;

  notifyResizeObserversFunction_ = nullptr;
  commitHookRegistered_ = false;
}

std::vector<ResizeObserverEntry> ResizeObserverManager::takeRecords() {
  std::unique_lock lock(pendingEntriesMutex_);

  notifiedResizeObservers_ = false;

  std::vector<ResizeObserverEntry> entries;
  pendingEntries_.swap(entries);
  return entries;
}

#pragma mark - UIManagerCommitHook

void ResizeObserverManager::commitHookWasRegistered(
    const UIManager& uiManager) noexcept {}
void ResizeObserverManager::commitHookWasUnregistered(
    const UIManager& uiManager) noexcept {}

void ResizeObserverManager::shadowTreeDidCommit(
    const ShadowTree& shadowTree,
    const RootShadowNode::Shared& /*rootShadowNode*/,
    const std::vector<const LayoutableShadowNode*>&
        affectedLayoutableNodes) noexcept {
  TraceSection s("ResizeObserverManager::shadowTreeDidCommit");

  // This runs on the commit hook, which may execute on any thread. It must
  // not compute observations or notify JS: it only collects which observed
  // targets went dirty, so that `runResizeObservations` (invoked from the
  // event-loop step, always on the JS thread) can compute their new sizes
  // exactly once against the latest committed tree.
  auto surfaceId = shadowTree.getSurfaceId();

  std::unordered_set<const ShadowNodeFamily*> observedFamilies;
  {
    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      // No observers registered for this surface: nothing to collect.
      return;
    }

    observedFamilies.reserve(observersIt->second.size());
    for (auto& observer : observersIt->second) {
      observedFamilies.insert(observer->getTargetShadowNodeFamily().get());
    }
  }

  std::unordered_set<const ShadowNodeFamily*> newlyDirtyFamilies;
  for (const auto* node : affectedLayoutableNodes) {
    const auto* family = &node->getFamily();
    if (observedFamilies.contains(family)) {
      newlyDirtyFamilies.insert(family);
    }
  }

  if (newlyDirtyFamilies.empty()) {
    return;
  }

  std::unique_lock lock(dirtyFamiliesMutex_);
  auto& dirtyFamilies = dirtyFamiliesBySurfaceId_[surfaceId];
  dirtyFamilies.insert(newlyDirtyFamilies.begin(), newlyDirtyFamilies.end());
}

#pragma mark - RuntimeSchedulerResizeObserverDelegate

void ResizeObserverManager::runResizeObservations(
    const std::unordered_set<
        SurfaceId>& /*surfaceIdsWithPendingRenderingUpdates*/) {
  TraceSection s("ResizeObserverManager::runResizeObservations");

  // Drain the dirty families collected by the commit hook since the last
  // time this step ran.
  std::unordered_map<SurfaceId, std::unordered_set<const ShadowNodeFamily*>>
      dirtyFamiliesBySurfaceId;
  {
    std::unique_lock lock(dirtyFamiliesMutex_);
    dirtyFamiliesBySurfaceId.swap(dirtyFamiliesBySurfaceId_);
  }

  // A surface is relevant to this step if either (a) it has dirty families
  // collected from a commit, or (b) it has observers awaiting their first
  // delivery check (e.g. observers just registered via `observe()`).
  std::unordered_set<SurfaceId> candidateSurfaceIds;
  for (const auto& [surfaceId, families] : dirtyFamiliesBySurfaceId) {
    candidateSurfaceIds.insert(surfaceId);
  }

  {
    std::unique_lock lock(observersMutex_);
    for (const auto& [surfaceId, observers] : observersBySurfaceId_) {
      for (const auto& observer : observers) {
        if (observer->needsInitialDeliveryCheck()) {
          candidateSurfaceIds.insert(surfaceId);
          break;
        }
      }
    }
  }

  if (candidateSurfaceIds.empty()) {
    return;
  }

  std::vector<ResizeObserverEntry> entries;

  for (auto surfaceId : candidateSurfaceIds) {
    RootShadowNode::Shared rootShadowNode =
        getRootShadowNode(surfaceId, *shadowTreeRegistry_);
    if (rootShadowNode == nullptr) {
      continue;
    }

    auto dirtyFamiliesIt = dirtyFamiliesBySurfaceId.find(surfaceId);
    const std::unordered_set<const ShadowNodeFamily*>* dirtyFamilies =
        dirtyFamiliesIt != dirtyFamiliesBySurfaceId.end()
        ? &dirtyFamiliesIt->second
        : nullptr;

    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      continue;
    }

    for (auto& observer : observersIt->second) {
      // Only recompute observers whose target was affected by a commit
      // since the last pass, or that still need their first delivery check.
      bool wasAffected = dirtyFamilies != nullptr &&
          dirtyFamilies->contains(observer->getTargetShadowNodeFamily().get());
      if (!wasAffected && !observer->needsInitialDeliveryCheck()) {
        continue;
      }

      auto entry = observer->updateStateIfNeeded(*rootShadowNode);
      if (entry) {
        entries.push_back(std::move(entry).value());
      }
    }
  }

  if (entries.empty()) {
    return;
  }

  {
    std::unique_lock lock(pendingEntriesMutex_);
    pendingEntries_.insert(
        pendingEntries_.end(), entries.begin(), entries.end());
  }

  notifyObserversIfNecessary();
}

#pragma mark - Private methods

void ResizeObserverManager::notifyObserversIfNecessary() {
  bool dispatchNotification = false;

  {
    std::unique_lock lock(pendingEntriesMutex_);

    if (!pendingEntries_.empty() && !notifiedResizeObservers_) {
      notifiedResizeObservers_ = true;
      dispatchNotification = true;
    }
  }

  if (dispatchNotification) {
    notifyObservers();
  }
}

void ResizeObserverManager::notifyObservers() {
  TraceSection s("ResizeObserverManager::notifyObservers");
  if (notifyResizeObserversFunction_) {
    notifyResizeObserversFunction_();
  }
}

} // namespace facebook::react
