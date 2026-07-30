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
  TraceSection s{"ResizeObserverManager::observe"};

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  // Per spec, new targets are delivered on the next "update the rendering"
  // step (`runResizeObservations`), not here. That step runs at the end of
  // every task, so the first delivery is still prompt.
  {
    std::unique_lock lock{observersMutex_};

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
  TraceSection s{"ResizeObserverManager::unobserve"};

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  // If another observer still targets this family, keep its dirty state.
  auto familyStillObserved = false;
  {
    std::unique_lock lock{observersMutex_};

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

  // Clean up the removed observation to avoid a wasted pass, a stale entry, or
  // a dangling family pointer. Only clear the dirty family if no other
  // observer targets it.
  if (!familyStillObserved) {
    std::unique_lock lock{dirtyFamiliesMutex_};
    auto dirtyFamiliesIt = dirtyFamiliesBySurfaceId_.find(surfaceId);
    if (dirtyFamiliesIt != dirtyFamiliesBySurfaceId_.end()) {
      dirtyFamiliesIt->second.erase(shadowNodeFamily.get());
      if (dirtyFamiliesIt->second.empty()) {
        dirtyFamiliesBySurfaceId_.erase(dirtyFamiliesIt);
      }
    }
  }

  {
    std::unique_lock lock{pendingEntriesMutex_};
    pendingEntries_.erase(
        std::remove_if(
            pendingEntries_.begin(),
            pendingEntries_.end(),
            [resizeObserverId, &shadowNodeFamily](const auto& entry) {
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
  TraceSection s{"ResizeObserverManager::connect"};

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
  TraceSection s{"ResizeObserverManager::disconnect"};

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
  std::unique_lock lock{pendingEntriesMutex_};

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
  TraceSection s{"ResizeObserverManager::shadowTreeDidCommit"};

  // Runs on the commit hook (any thread), so it only collects which observed
  // targets went dirty; it must not compute observations or notify JS.
  // `runResizeObservations` (JS thread) does that against the latest tree.
  auto surfaceId = shadowTree.getSurfaceId();

  std::unordered_set<const ShadowNodeFamily*> observedFamilies;
  {
    std::unique_lock lock{observersMutex_};

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      // No observers for this surface.
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

  std::unique_lock lock{dirtyFamiliesMutex_};
  // Record the commit even with no dirty family, so `runResizeObservations`
  // re-checks for removed targets (they aren't in `affectedLayoutableNodes`).
  committedSurfaceIds_.insert(surfaceId);
  if (!newlyDirtyFamilies.empty()) {
    auto& dirtyFamilies = dirtyFamiliesBySurfaceId_[surfaceId];
    dirtyFamilies.insert(newlyDirtyFamilies.begin(), newlyDirtyFamilies.end());
  }
}

#pragma mark - RuntimeSchedulerResizeObserverDelegate

void ResizeObserverManager::runResizeObservations() {
  TraceSection s{"ResizeObserverManager::runResizeObservations"};

  // Drain what the commit hook collected since the last run.
  std::unordered_map<SurfaceId, std::unordered_set<const ShadowNodeFamily*>>
      dirtyFamiliesBySurfaceId;
  std::unordered_set<SurfaceId> committedSurfaceIds;
  {
    std::unique_lock lock{dirtyFamiliesMutex_};
    dirtyFamiliesBySurfaceId.swap(dirtyFamiliesBySurfaceId_);
    committedSurfaceIds.swap(committedSurfaceIds_);
  }

  // A surface is relevant if it committed (layout changes or removals) or has
  // an observer awaiting first delivery. Sourcing the latter from the pending
  // set lets a quiet tick skip scanning all observers.
  std::unordered_set<SurfaceId> candidateSurfaceIds{committedSurfaceIds};
  candidateSurfaceIds.insert(
      surfaceIdsWithPendingInitialDelivery_.begin(),
      surfaceIdsWithPendingInitialDelivery_.end());

  if (candidateSurfaceIds.empty()) {
    return;
  }

  std::vector<PendingObservation> pendingObservations;

  for (auto surfaceId : candidateSurfaceIds) {
    auto rootShadowNode = std::shared_ptr<const RootShadowNode>{};
    shadowTreeRegistry_->visit(surfaceId, [&](const auto& shadowTree) {
      rootShadowNode = shadowTree.getCurrentRevision().rootShadowNode;
    });
    if (rootShadowNode == nullptr) {
      continue;
    }

    auto dirtyFamiliesIt = dirtyFamiliesBySurfaceId.find(surfaceId);
    const auto* dirtyFamilies =
        dirtyFamiliesIt != dirtyFamiliesBySurfaceId.end()
        ? &dirtyFamiliesIt->second
        : nullptr;

    auto surfaceCommitted = committedSurfaceIds.contains(surfaceId);

    std::unique_lock lock{observersMutex_};

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      continue;
    }

    // Whether an observer here still awaits first delivery after this pass
    // (e.g. reset because it has no reportable size), so the surface stays
    // pending.
    auto stillNeedsInitialDelivery = false;

    for (auto& observer : observersIt->second) {
      // Identity comparison only; see `dirtyFamiliesBySurfaceId_`.
      auto wasAffected = dirtyFamilies != nullptr &&
          dirtyFamilies->contains(observer->getTargetShadowNodeFamily().get());

      // Re-check delivered observations on any commit to catch targets that
      // left the tree (removals aren't dirty families) and send their final
      // 0x0. Skip ones already settled as detached - reinsertion comes via the
      // dirty path.
      auto maybeDetached = surfaceCommitted &&
          !observer->needsInitialDeliveryCheck() &&
          !observer->hasDeliveredDetachedState();

      // Recompute only observers that were affected, still need first
      // delivery, or may have just detached.
      if (!wasAffected && !observer->needsInitialDeliveryCheck() &&
          !maybeDetached) {
        continue;
      }

      auto entry = observer->updateStateIfNeeded(*rootShadowNode);
      if (entry) {
        pendingObservations.push_back(
            PendingObservation{
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

  // Deliver in spec order: observers by registration, then targets by
  // `observe()` order. Depth doesn't matter here — in the spec it only gates
  // the re-layout loop (which RN doesn't run), not entry order.
  std::sort(
      pendingObservations.begin(),
      pendingObservations.end(),
      [](const auto& a, const auto& b) {
        if (a.resizeObserverId != b.resizeObserverId) {
          return a.resizeObserverId < b.resizeObserverId;
        }
        return a.observationSequence < b.observationSequence;
      });

  {
    std::unique_lock lock{pendingEntriesMutex_};
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
  auto dispatchNotification = false;

  {
    std::unique_lock lock{pendingEntriesMutex_};

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
  TraceSection s{"ResizeObserverManager::notifyObservers"};
  if (notifyResizeObserversFunction_) {
    notifyResizeObserversFunction_();
  }
}

} // namespace facebook::react
