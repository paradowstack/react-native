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
    UIManager& uiManager) {
  TraceSection s("ResizeObserverManager::observe");

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  // Per spec, `observe` must perform an initial observation synchronously,
  // using the shadow tree that is current at the time `observe` is called
  // (i.e., independent of the commit hook, which only runs for subsequent
  // commits).
  RootShadowNode::Shared rootShadowNode =
      getRootShadowNode(surfaceId, uiManager.getShadowTreeRegistry());

  std::optional<ResizeObserverEntry> entry;

  {
    std::unique_lock lock(observersMutex_);

    auto& observers = observersBySurfaceId_[surfaceId];

    observers.emplace_back(std::make_unique<ResizeObserver>(
        resizeObserverId, shadowNodeFamily, boxOptions));

    auto* observer = observers.back().get();

    if (rootShadowNode != nullptr) {
      entry = observer->updateStateIfNeeded(*rootShadowNode);
    }
  }

  if (entry) {
    {
      std::unique_lock lock(pendingEntriesMutex_);
      pendingEntries_.push_back(std::move(entry).value());
    }
    notifyObserversIfNecessary();
  }
}

void ResizeObserverManager::unobserve(
    ResizeObserverObserverId resizeObserverId,
    const ShadowNodeFamily::Shared& shadowNodeFamily) {
  TraceSection s("ResizeObserverManager::unobserve");

  std::unique_lock lock(observersMutex_);

  auto surfaceId = shadowNodeFamily->getSurfaceId();

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

  if (observers.empty()) {
    observersBySurfaceId_.erase(surfaceId);
  }
}

void ResizeObserverManager::connect(
    UIManager& uiManager,
    std::function<void()> notifyResizeObserversFunction) {
  TraceSection s("ResizeObserverManager::connect");

  // Fail-safe in case the caller doesn't guarantee consistency.
  if (commitHookRegistered_) {
    return;
  }

  notifyResizeObserversFunction_ = std::move(notifyResizeObserversFunction);

  uiManager.registerCommitHook(*this);
  commitHookRegistered_ = true;
}

void ResizeObserverManager::disconnect(UIManager& uiManager) {
  TraceSection s("ResizeObserverManager::disconnect");

  // Fail-safe in case the caller doesn't guarantee consistency.
  if (!commitHookRegistered_) {
    return;
  }

  uiManager.unregisterCommitHook(*this);

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
    const RootShadowNode::Shared& rootShadowNode,
    const std::vector<const LayoutableShadowNode*>&
        affectedLayoutableNodes) noexcept {
  runResizeObservations(shadowTree, *rootShadowNode, affectedLayoutableNodes);
}

#pragma mark - Private methods

void ResizeObserverManager::runResizeObservations(
    const ShadowTree& shadowTree,
    const RootShadowNode& rootShadowNode,
    const std::vector<const LayoutableShadowNode*>& affectedLayoutableNodes) {
  TraceSection s("ResizeObserverManager::runResizeObservations");

  auto surfaceId = shadowTree.getSurfaceId();

  // Build a set of the families whose layout actually changed in this
  // commit, so we only recompute observers that could possibly have a new
  // size.
  std::unordered_set<const ShadowNodeFamily*> affectedFamilies;
  affectedFamilies.reserve(affectedLayoutableNodes.size());
  for (const auto* node : affectedLayoutableNodes) {
    affectedFamilies.insert(&node->getFamily());
  }

  std::vector<ResizeObserverEntry> entries;

  {
    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      return;
    }

    auto& observers = observersIt->second;
    for (auto& observer : observers) {
      // Only recompute observers whose target was affected by this commit,
      // or that have never reported yet (e.g. an initial observation that
      // raced with a commit before it could run).
      bool wasAffected = affectedFamilies.contains(
          observer->getTargetShadowNodeFamily().get());
      if (!wasAffected && observer->hasReported()) {
        continue;
      }

      auto entry = observer->updateStateIfNeeded(rootShadowNode);
      if (entry) {
        entries.push_back(std::move(entry).value());
      }
    }
  }

  {
    std::unique_lock lock(pendingEntriesMutex_);
    pendingEntries_.insert(
        pendingEntries_.end(), entries.begin(), entries.end());
  }

  notifyObserversIfNecessary();
}

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
