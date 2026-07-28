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

struct PendingObservation {
  ResizeObserverEntry entry;
  ResizeObserverObserverId resizeObserverId;
  uint64_t observationSequence;
};
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
  {
    std::unique_lock lock(observersMutex_);

    auto& observers = observersBySurfaceId_[surfaceId];

    observers.emplace_back(
        std::make_unique<ResizeObserver>(
            resizeObserverId,
            shadowNodeFamily,
            boxOptions,
            nextObservationSequence_++));
  }

  surfaceIdsWithPendingInitialDelivery_.insert(surfaceId);
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
      surfaceIdsWithPendingInitialDelivery_.erase(surfaceId);
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

  std::unique_lock lock(dirtyFamiliesMutex_);
  // Always record the commit, even when no observed family changed layout, so
  // `runResizeObservations` re-checks this surface's observers for targets that
  // left the tree (removals are not reported in `affectedLayoutableNodes`).
  committedSurfaceIds_.insert(surfaceId);
  if (!newlyDirtyFamilies.empty()) {
    auto& dirtyFamilies = dirtyFamiliesBySurfaceId_[surfaceId];
    dirtyFamilies.insert(newlyDirtyFamilies.begin(), newlyDirtyFamilies.end());
  }
}

#pragma mark - RuntimeSchedulerResizeObserverDelegate

void ResizeObserverManager::runResizeObservations() {
  TraceSection s("ResizeObserverManager::runResizeObservations");

  // Drain the dirty families collected by the commit hook since the last
  // time this step ran.
  std::unordered_map<SurfaceId, std::unordered_set<const ShadowNodeFamily*>>
      dirtyFamiliesBySurfaceId;
  std::unordered_set<SurfaceId> committedSurfaceIds;
  {
    std::unique_lock lock(dirtyFamiliesMutex_);
    dirtyFamiliesBySurfaceId.swap(dirtyFamiliesBySurfaceId_);
    committedSurfaceIds.swap(committedSurfaceIds_);
  }

  // A surface is relevant to this step if either (a) it committed since the
  // last pass (which covers both layout changes and target removals), or
  // (b) it has observers awaiting their first delivery check (e.g. observers
  // just registered via `observe()`). (b) comes straight from
  // `surfaceIdsWithPendingInitialDelivery_` rather than a scan of
  // `observersBySurfaceId_`, so a steady-state tick with nothing dirty and
  // every observer already delivered does not need to lock `observersMutex_`
  // at all.
  std::unordered_set<SurfaceId> candidateSurfaceIds(committedSurfaceIds);
  candidateSurfaceIds.insert(
      surfaceIdsWithPendingInitialDelivery_.begin(),
      surfaceIdsWithPendingInitialDelivery_.end());

  if (candidateSurfaceIds.empty()) {
    return;
  }

  std::vector<PendingObservation> pendingObservations;

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

    bool surfaceCommitted = committedSurfaceIds.contains(surfaceId);

    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      continue;
    }

    // Whether, after this pass, this surface still has an observer awaiting
    // its first delivery — e.g. it just registered mid-pass and has yet to
    // resolve, or `updateStateIfNeeded` reset it back to pending (an
    // `EmptyLayoutMetrics` result). Determines whether the surface stays in
    // `surfaceIdsWithPendingInitialDelivery_` for the next tick.
    bool stillNeedsInitialDelivery = false;

    for (auto& observer : observersIt->second) {
      // See `dirtyFamiliesBySurfaceId_`: identity comparison only, never
      // dereferenced.
      bool wasAffected = dirtyFamilies != nullptr &&
          dirtyFamilies->contains(observer->getTargetShadowNodeFamily().get());

      // Re-check a delivered observation whenever its surface committed, to
      // catch a target that left the tree (removals don't show up as dirty
      // families) and deliver its final 0x0 entry. Skip observations already
      // settled as detached: their reinsertion arrives via the dirty path.
      bool maybeDetached = surfaceCommitted &&
          !observer->needsInitialDeliveryCheck() &&
          !observer->hasDeliveredDetachedState();

      // Only recompute observers whose target was affected by a commit since
      // the last pass, that still need their first delivery check, or that
      // may have just detached.
      if (!wasAffected && !observer->needsInitialDeliveryCheck() &&
          !maybeDetached) {
        continue;
      }

      auto entry = observer->updateStateIfNeeded(*rootShadowNode);
      if (entry) {
        pendingObservations.push_back(PendingObservation{
            .entry = std::move(entry).value(),
            .resizeObserverId = observer->getResizeObserverId(),
            .observationSequence = observer->getObservationSequence(),
        });
      }

      stillNeedsInitialDelivery =
          stillNeedsInitialDelivery || observer->needsInitialDeliveryCheck();
    }

    if (stillNeedsInitialDelivery) {
      surfaceIdsWithPendingInitialDelivery_.insert(surfaceId);
    } else {
      surfaceIdsWithPendingInitialDelivery_.erase(surfaceId);
    }
  }

  if (pendingObservations.empty()) {
    return;
  }

  // Deliver in the order the spec's algorithms imply by iterating their
  // ordered lists: observers in registration order (`[[resizeObservers]]`),
  // and within an observer its targets in `observe()` order
  // (`[[observationTargets]]`). Depth is deliberately not a factor: in the
  // spec it only gates the rounds of the (re-layout) broadcast loop, which
  // RN does not run, and never reorders entries within a callback.
  std::sort(
      pendingObservations.begin(),
      pendingObservations.end(),
      [](const PendingObservation& a, const PendingObservation& b) {
        if (a.resizeObserverId != b.resizeObserverId) {
          return a.resizeObserverId < b.resizeObserverId;
        }
        return a.observationSequence < b.observationSequence;
      });

  {
    std::unique_lock lock(pendingEntriesMutex_);
    pendingEntries_.reserve(
        pendingEntries_.size() + pendingObservations.size());
    for (auto& observation : pendingObservations) {
      pendingEntries_.push_back(std::move(observation.entry));
    }
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
