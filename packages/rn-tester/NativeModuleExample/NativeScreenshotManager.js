/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

import type {CodegenTypes, TurboModule} from 'react-native';

import {TurboModuleRegistry} from 'react-native';

export type ScreenshotManagerOptions = CodegenTypes.UnsafeObject;

export type ValueStruct = {
  x: number,
  y: string,
  buffer: ArrayBuffer,
};

export interface Spec extends TurboModule {
  takeScreenshot(
    id: string,
    options: ScreenshotManagerOptions,
  ): Promise<string>;
  getBuffer(): ArrayBuffer;
  processBuffer(buffer: ArrayBuffer): string;
  lazyBuffer(): Promise<ArrayBuffer>;
  getValue: (x: number, y: string, a: ArrayBuffer) => ValueStruct;
}

export default (TurboModuleRegistry.get<Spec>('ScreenshotManager'): ?Spec);
