/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

/**
 * This module handles the communication between the React Native renderer
 * and all the resize observers that are currently observing any targets.
 *
 * In order to reduce the communication between native and JavaScript,
 * we register a single notification callback in native, and then we handle
 * how to notify each entry to the right resize observer when we receive all
 * the notifications together.
 *
 * TODO(ResizeObserver): this module is a stub. None of the exported
 * functions communicate with native yet.
 */

import type ReactNativeElement from '../../dom/nodes/ReactNativeElement';
import type ResizeObserver, {
  ResizeObserverBoxOptions,
  ResizeObserverCallback,
} from '../ResizeObserver';

// TODO(ResizeObserver): uncomment once the native module is wired up.
// import NativeResizeObserver from '../specs/NativeResizeObserver';

export type ResizeObserverId = number;

let nextResizeObserverId: ResizeObserverId = 1;

const registeredResizeObservers: Map<
  ResizeObserverId,
  {observer: ResizeObserver, callback: ResizeObserverCallback},
> = new Map();

/**
 * Registers the given resize observer and returns a unique ID for it, which
 * is required to start observing targets.
 */
export function registerObserver(
  observer: ResizeObserver,
  callback: ResizeObserverCallback,
): ResizeObserverId {
  const resizeObserverId = nextResizeObserverId;
  nextResizeObserverId++;
  registeredResizeObservers.set(resizeObserverId, {
    observer,
    callback,
  });
  return resizeObserverId;
}

/**
 * Unregisters the given resize observer.
 * This should only be called when an observer is no longer observing any
 * targets.
 */
export function unregisterObserver(resizeObserverId: ResizeObserverId): void {
  // TODO(ResizeObserver): implement. Once there are no observers left, this
  // should disconnect the native module (mirroring
  // `IntersectionObserverManager.unregisterObserver`).
  registeredResizeObservers.delete(resizeObserverId);
}

/**
 * Starts observing a target on a specific resize observer.
 * If this is the first target being observed, this should also set up the
 * centralized notification callback in native.
 */
export function observe({
  resizeObserverId,
  target,
  box,
}: {
  resizeObserverId: ResizeObserverId,
  target: ReactNativeElement,
  box: ResizeObserverBoxOptions,
}): void {
  // TODO(ResizeObserver): implement.
}

/**
 * Instructs the given resize observer to stop observing the specified
 * target.
 */
export function unobserve(
  resizeObserverId: ResizeObserverId,
  target: ReactNativeElement,
): void {
  // TODO(ResizeObserver): implement.
}

// TODO(ResizeObserver): implement a `notifyResizeObservers` function that
// will be called from native when there are `ResizeObserver` entries to
// dispatch. It should read the pending entries from
// `NativeResizeObserver.takeRecords()`, group them by observer, and invoke
// each observer's callback (mirroring
// `IntersectionObserverManager.notifyIntersectionObservers` /
// `doNotifyIntersectionObservers`).
