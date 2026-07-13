/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

// flowlint unsafe-getters-setters:off

import {setPlatformObject} from '../webidl/PlatformObjects';

/**
 * The [`ResizeObserverSize`](https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserverSize)
 * interface of the Resize Observer API is used to store the block and inline
 * sizes of a box as separate properties.
 *
 * It is returned by the `contentBoxSize` and `borderBoxSize` properties of
 * `ResizeObserverEntry`.
 */
export default class ResizeObserverSize {
  _inlineSize: number;
  _blockSize: number;

  constructor(inlineSize: number, blockSize: number) {
    this._inlineSize = inlineSize;
    this._blockSize = blockSize;
  }

  /**
   * The length of the observed element's border box, in the inline
   * dimension. For boxes with a horizontal writing-mode, this is the vertical
   * dimension, or height; if the writing-mode is vertical, this is the
   * horizontal dimension, or width.
   */
  get inlineSize(): number {
    return this._inlineSize;
  }

  /**
   * The length of the observed element's border box, in the block dimension.
   * For boxes with a horizontal writing-mode, this is the vertical
   * dimension, or height; if the writing-mode is vertical, this is the
   * horizontal dimension, or width.
   */
  get blockSize(): number {
    return this._blockSize;
  }
}

setPlatformObject(ResizeObserverSize);

export function createResizeObserverSize(
  inlineSize: number,
  blockSize: number,
): ResizeObserverSize {
  return new ResizeObserverSize(inlineSize, blockSize);
}
