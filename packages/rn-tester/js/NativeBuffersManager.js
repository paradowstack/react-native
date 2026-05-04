import type {TurboModule} from 'react-native';

import {TurboModuleRegistry} from 'react-native';

export interface Spec extends TurboModule {
  processBuffer(buffer: ArrayBuffer): void;
  processBase64(buffer: string): void;
}

export default TurboModuleRegistry
      .getEnforcing<Spec>('BuffersManager');
