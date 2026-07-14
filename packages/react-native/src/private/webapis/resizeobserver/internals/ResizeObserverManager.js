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
 */

import type ReactNativeElement from '../../dom/nodes/ReactNativeElement';
import type ResizeObserver, {
  ResizeObserverBoxOptions,
  ResizeObserverCallback,
} from '../ResizeObserver';
import type ResizeObserverEntry from '../ResizeObserverEntry';

import {trace} from '../../../../../Libraries/Performance/Systrace';
import {
  getInstanceHandle,
  getNativeNodeReference,
} from '../../dom/nodes/internals/NodeInternals';
import {createResizeObserverEntry} from '../ResizeObserverEntry';
import NativeResizeObserver from '../specs/NativeResizeObserver';

export type ResizeObserverId = number;

let nextResizeObserverId: ResizeObserverId = 1;
let isConnected: boolean = false;

const registeredResizeObservers: Map<
  ResizeObserverId,
  {observer: ResizeObserver, callback: ResizeObserverCallback},
> = new Map();

// We need to keep the mapping from instance handles to targets because when
// targets are detached (their components are unmounted), React resets the
// instance handle to prevent memory leaks and it cuts the connection between
// the instance handle and the target.
const instanceHandleToTargetMap: WeakMap<interface {}, ReactNativeElement> =
  new WeakMap();

function getTargetFromInstanceHandle(
  instanceHandle: unknown,
): ?ReactNativeElement {
  // $FlowExpectedError[incompatible-type] instanceHandle is typed as mixed but we know it's an object and we need it to be to use it as a key in a WeakMap.
  const key: interface {} = instanceHandle;
  return instanceHandleToTargetMap.get(key);
}

function setTargetForInstanceHandle(
  instanceHandle: unknown,
  target: ReactNativeElement,
): void {
  // $FlowExpectedError[incompatible-type] instanceHandle is typed as mixed but we know it's an object and we need it to be to use it as a key in a WeakMap.
  const key: interface {} = instanceHandle;
  instanceHandleToTargetMap.set(key, target);
}

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
  const deleted = registeredResizeObservers.delete(resizeObserverId);
  if (deleted && registeredResizeObservers.size === 0) {
    NativeResizeObserver?.disconnect();
    isConnected = false;
  }
}

/**
 * Starts observing a target on a specific resize observer.
 * If this is the first target being observed, this also sets up the
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
  if (NativeResizeObserver == null) {
    throwIfNoNativeResizeObserver();
    return;
  }

  const registeredObserver = registeredResizeObservers.get(resizeObserverId);
  if (registeredObserver == null) {
    console.error(
      `ResizeObserverManager: could not start observing target because ResizeObserver with ID ${resizeObserverId} was not registered.`,
    );
    return;
  }

  const targetNativeNodeReference = getNativeNodeReference(target);
  if (targetNativeNodeReference == null) {
    // The target is disconnected. We can't observe it anymore.
    return;
  }

  const instanceHandle = getInstanceHandle(target);
  if (instanceHandle == null) {
    console.error(
      'ResizeObserverManager: could not find reference to instance handle from target',
    );
    return;
  }

  // Store the mapping between the instance handle and the target so we can
  // access it even after the instance handle has been unmounted.
  setTargetForInstanceHandle(instanceHandle, target);

  if (!isConnected) {
    NativeResizeObserver.connect(notifyResizeObservers);
    isConnected = true;
  }

  NativeResizeObserver.observe({
    resizeObserverId,
    targetShadowNode: targetNativeNodeReference,
    box,
  });
}

/**
 * Instructs the given resize observer to stop observing the specified
 * target.
 */
export function unobserve(
  resizeObserverId: ResizeObserverId,
  target: ReactNativeElement,
): void {
  if (NativeResizeObserver == null) {
    throwIfNoNativeResizeObserver();
    return;
  }

  const registeredObserver = registeredResizeObservers.get(resizeObserverId);
  if (registeredObserver == null) {
    console.error(
      `ResizeObserverManager: could not stop observing target because ResizeObserver with ID ${resizeObserverId} was not registered.`,
    );
    return;
  }

  const targetNativeNodeReference = getNativeNodeReference(target);
  if (targetNativeNodeReference == null) {
    // The target is already disconnected, so native doesn't have anything
    // to unobserve.
    return;
  }

  NativeResizeObserver.unobserve(resizeObserverId, targetNativeNodeReference);
}

/**
 * This function is called from native when there are `ResizeObserver`
 * entries to dispatch.
 */
function notifyResizeObservers(): void {
  trace('ResizeObserverManager.notifyResizeObservers', doNotifyResizeObservers);
}

function doNotifyResizeObservers(): void {
  if (NativeResizeObserver == null) {
    throwIfNoNativeResizeObserver();
    return;
  }

  const nativeEntries = NativeResizeObserver.takeRecords();

  const entriesByObserver: Map<
    ResizeObserverId,
    Array<ResizeObserverEntry>,
  > = new Map();

  for (const nativeEntry of nativeEntries) {
    let list = entriesByObserver.get(nativeEntry.resizeObserverId);
    if (list == null) {
      list = [];
      entriesByObserver.set(nativeEntry.resizeObserverId, list);
    }

    const target = getTargetFromInstanceHandle(
      nativeEntry.targetInstanceHandle,
    );
    if (target == null) {
      console.warn('Could not find target to create ResizeObserverEntry');
      continue;
    }

    list.push(createResizeObserverEntry(nativeEntry, target));
  }

  for (const [resizeObserverId, entriesForObserver] of entriesByObserver) {
    const registeredObserver = registeredResizeObservers.get(resizeObserverId);
    if (!registeredObserver) {
      // This could happen if the observer is disconnected between commit
      // and mount. In this case, we can just ignore the entries.
      continue;
    }

    const {observer, callback} = registeredObserver;
    try {
      callback.call(observer, entriesForObserver, observer);
    } catch (error) {
      console.error(error);
    }
  }
}

function throwIfNoNativeResizeObserver() {
  throw new Error('Missing native implementation of ResizeObserver');
}
