/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ResizeObserverManager.h"
#include <cxxreact/TraceSection.h>
#include <algorithm>
#include <utility>
#include "ResizeObserver.h"

namespace facebook::react {

ResizeObserverManager::ResizeObserverManager() = default;

void ResizeObserverManager::observe(
    ResizeObserverObserverId resizeObserverId,
    const ShadowNodeFamily::Shared& shadowNodeFamily,
    ResizeObserverBoxOptions boxOptions,
    UIManager& /*uiManager*/) {
  TraceSection s("ResizeObserverManager::observe");

  auto surfaceId = shadowNodeFamily->getSurfaceId();

  std::unique_lock lock(observersMutex_);

  auto& observers = observersBySurfaceId_[surfaceId];

  observers.emplace_back(
      std::make_unique<ResizeObserver>(
          resizeObserverId, shadowNodeFamily, boxOptions));
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

RootShadowNode::Unshared ResizeObserverManager::shadowTreeWillCommit(
    const ShadowTree& shadowTree,
    const RootShadowNode::Shared& oldRootShadowNode,
    const RootShadowNode::Unshared& newRootShadowNode,
    const ShadowTree::CommitOptions& commitOptions) noexcept {
  if (commitOptions.source == ShadowTree::CommitSource::React) {
    runResizeObservations(shadowTree, *oldRootShadowNode, *newRootShadowNode);
  }
  return newRootShadowNode;
}

#pragma mark - Private methods

void ResizeObserverManager::runResizeObservations(
    const ShadowTree& shadowTree,
    const RootShadowNode& /*oldRootShadowNode*/,
    const RootShadowNode& newRootShadowNode) {
  TraceSection s("ResizeObserverManager::runResizeObservations");

  auto surfaceId = shadowTree.getSurfaceId();

  std::vector<ResizeObserverEntry> entries;

  {
    std::unique_lock lock(observersMutex_);

    auto observersIt = observersBySurfaceId_.find(surfaceId);
    if (observersIt == observersBySurfaceId_.end()) {
      return;
    }

    auto& observers = observersIt->second;
    for (auto& observer : observers) {
      // TODO(ResizeObserver): implement
      auto entry = observer->updateStateIfNeeded(newRootShadowNode);
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
  notifyResizeObserversFunction_();
}

} // namespace facebook::react
