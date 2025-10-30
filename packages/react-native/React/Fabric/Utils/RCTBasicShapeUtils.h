/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <react/renderer/graphics/ClipPath.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Utility functions for creating UIBezierPath instances from BasicShape variants.
 * These utilities can be used for clip-path, shape rendering, or any other feature
 * that needs to convert CSS basic shapes to CoreGraphics paths.
 */
@interface RCTBasicShapeUtils : NSObject

/**
 * Creates a UIBezierPath from a BasicShape variant.
 * Supports circle, ellipse, inset, polygon, rect, and xywh shapes.
 *
 * @param basicShape The BasicShape variant to convert
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the shape, or nil if the shape is invalid
 */
+ (UIBezierPath *_Nullable)createPathFromBasicShape:(const facebook::react::BasicShape &)basicShape
                                             bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for a circle shape.
 *
 * @param circle The circle shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the circle, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createCirclePath:(const facebook::react::CircleShape &)circle bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for an ellipse shape.
 *
 * @param ellipse The ellipse shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the ellipse, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createEllipsePath:(const facebook::react::EllipseShape &)ellipse bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for an inset shape.
 *
 * @param inset The inset shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the inset, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createInsetPath:(const facebook::react::InsetShape &)inset bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for a polygon shape.
 *
 * @param polygon The polygon shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the polygon, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createPolygonPath:(const facebook::react::PolygonShape &)polygon bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for a rect shape.
 * Note: CSS rect() uses distances from edges (top, right, bottom, left).
 *
 * @param rect The rect shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the rect, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createRectPath:(const facebook::react::RectShape &)rect bounds:(CGRect)bounds;

/**
 * Creates a UIBezierPath for an xywh shape.
 *
 * @param xywh The xywh shape definition
 * @param bounds The reference bounds for resolving percentage-based values
 * @return A UIBezierPath representing the xywh rectangle, or nil if invalid
 */
+ (UIBezierPath *_Nullable)createXywhPath:(const facebook::react::XywhShape &)xywh bounds:(CGRect)bounds;

@end

NS_ASSUME_NONNULL_END
