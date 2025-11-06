/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <React/RCTBorderDrawing.h>
#import <react/renderer/components/view/primitives.h>
#import <react/renderer/core/LayoutMetrics.h>
#import <react/renderer/graphics/ClipPath.h>
#import <yoga/style/Style.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCTClipPathUtils : NSObject

/**
 * Adjusts corner radii based on the geometry box type.
 * - MarginBox: extends radii by margin amount
 * - BorderBox: uses radii as-is (reference box)
 * - PaddingBox: reduces radii by border width
 * - ContentBox: reduces radii by border width + padding
 *
 * @param geometryBox The geometry box type
 * @param cornerRadii The original corner radii (based on border-box)
 * @param layoutMetrics The layout metrics containing border and content insets
 * @param yogaStyle The Yoga style containing margin values
 * @return The adjusted corner radii appropriate for the geometry box
 */
+ (RCTCornerRadii)adjustCornerRadiiForGeometryBox:(facebook::react::GeometryBox)geometryBox
                                      cornerRadii:(RCTCornerRadii)cornerRadii
                                    layoutMetrics:(const facebook::react::LayoutMetrics &)layoutMetrics
                                        yogaStyle:(const facebook::yoga::Style &)yogaStyle;

/**
 * Calculates the appropriate CGRect for a geometry box type.
 * Geometry boxes define the reference box for clip-path and other CSS properties.
 *
 * @param geometryBox The geometry box type (ContentBox, PaddingBox, BorderBox, MarginBox, etc.)
 * @param layoutMetrics The layout metrics containing frame information
 * @param yogaStyle The Yoga style containing margin, padding, and border values
 * @param bounds The view bounds to use as fallback for BorderBox
 * @return The calculated CGRect for the specified geometry box
 */
+ (CGRect)getGeometryBoxRect:(facebook::react::GeometryBox)geometryBox
               layoutMetrics:(const facebook::react::LayoutMetrics &)layoutMetrics
                   yogaStyle:(const facebook::yoga::Style &)yogaStyle
                      bounds:(CGRect)bounds;

/**
 * Creates a CALayer configured as a mask for the given clip-path.
 * Returns nil if the clip-path cannot be rendered (e.g., empty or invalid shape).
 *
 * @param clipPath The clip-path definition containing the shape and geometry box
 * @param layoutMetrics The layout metrics containing frame information
 * @param yogaStyle The Yoga style containing margin, padding, and border values
 * @param bounds The view bounds to use as fallback for BorderBox
 * @param cornerRadii The corner radii to consider when creating the clip path
 * @return A CALayer to be used as a mask layer, or nil if the clip-path is invalid
 */
+ (CALayer *_Nullable)createClipPathLayer:(const facebook::react::ClipPath &)clipPath
                            layoutMetrics:(const facebook::react::LayoutMetrics &)layoutMetrics
                                yogaStyle:(const facebook::yoga::Style &)yogaStyle
                                   bounds:(CGRect)bounds
                              cornerRadii:(RCTCornerRadii)cornerRadii;

@end

NS_ASSUME_NONNULL_END
