/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

import {CodegenTypes, TurboModule} from 'react-native';

import {TurboModuleRegistry} from 'react-native';

export type Union = {text: string} | {buffer: ArrayBuffer};

export type ArrayBufferStruct = {
  ocb: string,
  buffer: ArrayBuffer,
};

export type StringStruct = {
  text: ?string,
};

export type ConstantsStruct = {
  text: string,
  buffer: ArrayBuffer,
};

export interface Spec extends TurboModule {
  +onMyString: CodegenTypes.EventEmitter<string>;
  +onMyBuffer: CodegenTypes.EventEmitter<ArrayBuffer>;

  processBuffer(buffer: ArrayBuffer): void;
  getBuffer(): ArrayBuffer;
  getAsyncBuffer(): Promise<ArrayBuffer>;
  processBase64(buffer: string): void;
  processUnsafe(buffer: CodegenTypes.UnsafeObject): void;
  processUnion: (object: Union) => string;
  processArrayBufferStruct: (object: ArrayBufferStruct) => ?ArrayBuffer;
  processStringStruct: (object: StringStruct) => ?string;
  getConstants: () => ConstantsStruct;
  processArrayOfBuffers: (buffers: Array<ArrayBuffer>) => ?ArrayBuffer;
}

export default TurboModuleRegistry.getEnforcing<Spec>('BuffersManager');
