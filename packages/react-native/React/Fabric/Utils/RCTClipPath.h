/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <UIKit/UIKit.h>

#import <React/RCTDefines.h>
#import <react/renderer/graphics/ClipPath.h>

/**
 * Creates a CAShapeLayer configured as a mask for the given clip-path.
 * Returns nil if the clip-path cannot be rendered (e.g., empty shape).
 *
 * @param clipPath The clip-path definition containing the shape and geometry box
 * @param bounds The bounds of the view to apply the clip-path to
 * @return A CAShapeLayer to be used as a mask, or nil if the clip-path is invalid
 */
RCT_EXTERN CAShapeLayer *_Nullable RCTClipPathCreateMaskLayer(const facebook::react::ClipPath &clipPath, CGRect bounds);
