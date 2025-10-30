/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import "RCTClipPathUtils.h"

#import "RCTBasicShapeUtils.h"
#import <React/RCTConversions.h>

using namespace facebook::react;

@implementation RCTClipPathUtils

+ (CGRect)getGeometryBoxRect:(GeometryBox)geometryBox
               layoutMetrics:(const LayoutMetrics &)layoutMetrics
                   yogaStyle:(const facebook::yoga::Style &)yogaStyle
                      bounds:(CGRect)bounds
{
  auto marginLeft = yogaStyle.margin(facebook::yoga::Edge::Left).value().unwrapOrDefault(0.0f);
  auto marginRight = yogaStyle.margin(facebook::yoga::Edge::Right).value().unwrapOrDefault(0.0f);
  auto marginTop = yogaStyle.margin(facebook::yoga::Edge::Top).value().unwrapOrDefault(0.0f);
  auto marginBottom = yogaStyle.margin(facebook::yoga::Edge::Bottom).value().unwrapOrDefault(0.0f);
  
  switch (geometryBox) {
    case GeometryBox::ContentBox:
    case GeometryBox::FillBox:
      return RCTCGRectFromRect(layoutMetrics.getContentFrame());
    case GeometryBox::PaddingBox:
      return RCTCGRectFromRect(layoutMetrics.getPaddingFrame());
    case GeometryBox::BorderBox:
    case GeometryBox::StrokeBox:
    case GeometryBox::ViewBox:
      return bounds;
    case GeometryBox::MarginBox:
      return CGRectMake(
        bounds.origin.x - marginLeft,
        bounds.origin.y - marginTop,
        bounds.size.width + marginLeft + marginRight,
        bounds.size.height + marginTop + marginBottom
      );
  }
  
  return bounds;
}

+ (CALayer *)createClipPathLayer:(const ClipPath &)clipPath
                   layoutMetrics:(const LayoutMetrics &)layoutMetrics
                       yogaStyle:(const facebook::yoga::Style &)yogaStyle
                          bounds:(CGRect)bounds
                     cornerRadii:(RCTCornerRadii)cornerRadii
{
  // Calculate the geometry box rect if specified
  CGRect box = bounds;
  if (clipPath.geometryBox.has_value()) {
    box = [self getGeometryBoxRect:clipPath.geometryBox.value()
                     layoutMetrics:layoutMetrics
                         yogaStyle:yogaStyle
                            bounds:bounds];
  }
  
  UIBezierPath *path = nil;
  if (clipPath.shape.has_value()) {
    path = [RCTBasicShapeUtils createPathFromBasicShape:clipPath.shape.value() bounds:box];
  } else if (clipPath.geometryBox.has_value()) {
    // For geometry box only (no shape), create a rounded rectangle using border radius
    RCTCornerInsets cornerInsets = RCTGetCornerInsets(cornerRadii, UIEdgeInsetsZero);
    CGPathRef cgPath = RCTPathCreateWithRoundedRect(box, cornerInsets, nil, NO);
    path = [UIBezierPath bezierPathWithCGPath:cgPath];
    CGPathRelease(cgPath);
  }

  // If path creation failed or resulted in an invalid path, return nil
  if (path == nil) {
    return nil;
  }

  CAShapeLayer *maskLayer = [CAShapeLayer layer];
  maskLayer.path = path.CGPath;
  if (path.usesEvenOddFillRule) {
    maskLayer.fillRule = kCAFillRuleEvenOdd;
  }
  return maskLayer;
}

@end
