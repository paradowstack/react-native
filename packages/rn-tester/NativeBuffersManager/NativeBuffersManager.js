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

export type Union = {text: string} | {buffer: ArrayBuffer};

export type ArrayBufferStruct = {
  buffer: ?ArrayBuffer,
};

export type StringStruct = {
  text: ?string,
};

export interface Spec extends TurboModule {
  +onMyString: CodegenTypes.EventEmitter<string>;
  +onMyBuffer: CodegenTypes.EventEmitter<ArrayBuffer>;

  processBuffer(buffer: ArrayBuffer): void;
  processBase64(buffer: string): void;
  processUnion: (object: Union) => string;
  processArrayBufferStruct: (object: ArrayBufferStruct) => ?ArrayBuffer;
  processStringStruct: (object: StringStruct) => ?string;
}

export default TurboModuleRegistry.getEnforcing<Spec>('BuffersManager');
