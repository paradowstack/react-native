/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict
 * @format
 */

import type {TurboModule} from '../../../../../Libraries/TurboModule/RCTExport';

import * as TurboModuleRegistry from '../../../../../Libraries/TurboModule/TurboModuleRegistry';

export type NativeResizeObserverEntry = {
  resizeObserverId: number,
  targetInstanceHandle: unknown,
  contentRect: ReadonlyArray<number>, // It's actually a tuple with x, y, width and height
  borderBoxSize: ReadonlyArray<number>, // It's actually a tuple with inlineSize and blockSize
  contentBoxSize: ReadonlyArray<number>, // It's actually a tuple with inlineSize and blockSize
};

export type NativeResizeObserverObserveOptions = {
  resizeObserverId: number,
  targetShadowNode: unknown,
  // Corresponds to the `box` option of `ResizeObserver#observe`.
  box?: ?string,
};

export interface Spec extends TurboModule {
  readonly observe: (options: NativeResizeObserverObserveOptions) => void;
  readonly unobserve: (
    resizeObserverId: number,
    targetShadowNode: unknown,
  ) => void;
  readonly connect: (notifyResizeObserversFunction: () => void) => void;
  readonly disconnect: () => void;
  readonly takeRecords: () => ReadonlyArray<NativeResizeObserverEntry>;
}

export default TurboModuleRegistry.get<Spec>(
  'NativeResizeObserverCxx',
) as ?Spec;
