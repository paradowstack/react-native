/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/ShadowNodeFamily.h>
#include <react/renderer/mounting/ShadowTree.h>
#include <react/renderer/uimanager/UIManager.h>
#include <react/renderer/uimanager/UIManagerCommitHook.h>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "ResizeObserver.h"

namespace facebook::react {

class ResizeObserverManager final : public UIManagerCommitHook {
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
      UIManager& uiManager,
      std::function<void()> notifyResizeObserversFunction);

  void disconnect(UIManager& uiManager);

  std::vector<ResizeObserverEntry> takeRecords();

#pragma mark - UIManagerCommitHook

  void commitHookWasRegistered(const UIManager& uiManager) noexcept override;
  void commitHookWasUnregistered(const UIManager& uiManager) noexcept override;

  RootShadowNode::Unshared shadowTreeWillCommit(
      const ShadowTree& shadowTree,
      const RootShadowNode::Shared& oldRootShadowNode,
      const RootShadowNode::Unshared& newRootShadowNode,
      const ShadowTree::CommitOptions& commitOptions) noexcept override;

 private:
  mutable std::
      unordered_map<SurfaceId, std::vector<std::unique_ptr<ResizeObserver>>>
          observersBySurfaceId_;
  mutable std::mutex observersMutex_;

  std::function<void()> notifyResizeObserversFunction_;
  bool commitHookRegistered_{};

  mutable std::vector<ResizeObserverEntry> pendingEntries_;
  mutable std::mutex pendingEntriesMutex_;
  mutable bool notifiedResizeObservers_{};

  void runResizeObservations(
      const ShadowTree& shadowTree,
      const RootShadowNode& oldRootShadowNode,
      const RootShadowNode& newRootShadowNode);

  void notifyObserversIfNecessary();
  void notifyObservers();
};

} // namespace facebook::react
