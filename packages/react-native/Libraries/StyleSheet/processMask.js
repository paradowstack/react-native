/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

'use strict';

import type {
  MaskImageValue,
  MaskPositionValue,
  MaskRepeatValue,
  MaskSizeValue,
} from './StyleSheetTypes';

import processBackgroundImage from './processBackgroundImage';
import processBackgroundPosition from './processBackgroundPosition';
import processBackgroundRepeat from './processBackgroundRepeat';
import processBackgroundSize from './processBackgroundSize';

/**
 * Processes the mask-image CSS property.
 * Per CSS Masking Level 1 spec, mask-image has identical syntax to background-image:
 * https://www.w3.org/TR/css-masking-1/#the-mask-image
 *
 * Supports:
 * - Linear gradients: linear-gradient(...)
 * - Radial gradients: radial-gradient(...)
 * - URLs: url(...)
 * - Multiple comma-separated mask images
 *
 * @param maskImage - The mask-image value to process
 * @returns An array of processed mask image values
 */
export function processMaskImage(
  maskImage: ?($ReadOnlyArray<MaskImageValue> | string),
): $ReadOnlyArray<MaskImageValue> {
  // Reuse processBackgroundImage since mask-image has identical syntax
  // The spec explicitly states: "mask layer image" corresponds to "background images"
  return processBackgroundImage(maskImage);
}

/**
 * Processes the mask-size CSS property.
 * Per CSS Masking Level 1 spec, mask-size uses <bg-size># syntax:
 * https://www.w3.org/TR/css-masking-1/#the-mask-size
 *
 * Supports:
 * - Keywords: auto, cover, contain
 * - Length/percentage pairs: 100px 50%, 50% auto, etc.
 * - Multiple comma-separated mask sizes
 *
 * @param maskSize - The mask-size value to process
 * @returns An array of processed mask size values
 */
export function processMaskSize(
  maskSize: ?($ReadOnlyArray<MaskSizeValue> | string),
): $ReadOnlyArray<MaskSizeValue> {
  // Reuse processBackgroundSize since mask-size has identical syntax
  // The spec explicitly states: "mask-size" corresponds to "background-size"
  return processBackgroundSize(maskSize);
}

/**
 * Processes the mask-position CSS property.
 * Per CSS Masking Level 1 spec, mask-position uses <position># syntax:
 * https://www.w3.org/TR/css-masking-1/#the-mask-position
 *
 * Supports:
 * - Keywords: top, bottom, left, right, center
 * - Length/percentage values: 10px 20px, 50% 50%, etc.
 * - 4-value syntax: right 10px bottom 20px
 * - Multiple comma-separated mask positions
 *
 * @param maskPosition - The mask-position value to process
 * @returns An array of processed mask position values
 */
export function processMaskPosition(
  maskPosition: ?($ReadOnlyArray<MaskPositionValue> | string),
): $ReadOnlyArray<MaskPositionValue> {
  // Reuse processBackgroundPosition since mask-position has identical syntax
  // The spec explicitly states: "mask-position" corresponds to "background-position"
  return processBackgroundPosition(maskPosition);
}

/**
 * Processes the mask-repeat CSS property.
 * Per CSS Masking Level 1 spec, mask-repeat uses <repeat-style># syntax:
 * https://www.w3.org/TR/css-masking-1/#the-mask-repeat
 *
 * Supports:
 * - Keywords: repeat, space, round, no-repeat
 * - 1-value syntax: repeat (applies to both x and y)
 * - 2-value syntax: repeat no-repeat (x and y separately)
 * - Shorthand keywords: repeat-x, repeat-y
 * - Multiple comma-separated mask repeat values
 *
 * @param maskRepeat - The mask-repeat value to process
 * @returns An array of processed mask repeat values
 */
export function processMaskRepeat(
  maskRepeat: ?($ReadOnlyArray<MaskRepeatValue> | string),
): $ReadOnlyArray<MaskRepeatValue> {
  // Reuse processBackgroundRepeat since mask-repeat has identical syntax
  // The spec explicitly states mask-repeat uses same repeat-style as background-repeat
  return processBackgroundRepeat(maskRepeat);
}
