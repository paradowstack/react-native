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

export interface Spec extends TurboModule {
  +onMyString: CodegenTypes.EventEmitter<string>;
  +onMyBuffer: CodegenTypes.EventEmitter<ArrayBuffer>;

  processBuffer(buffer: ArrayBuffer): void;
  processBase64(buffer: string): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('BuffersManager');
