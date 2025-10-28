/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import "RCTClipPath.h"

#import <React/RCTConversions.h>

using namespace facebook::react;

/**
 * Resolves a ValueUnit to a CGFloat value based on a reference dimension.
 * For percentage units, calculates the value relative to the reference dimension.
 * For absolute units (pixels), returns the value directly.
 *
 * @param unit The ValueUnit to resolve
 * @param referenceDimension The dimension to use for percentage calculations
 * @return The resolved CGFloat value
 */
static CGFloat RCTResolveValueUnit(const ValueUnit &unit, CGFloat referenceDimension)
{
  if (unit.unit == UnitType::Percent) {
    return (CGFloat)unit.value * referenceDimension / 100.0f;
  }
  return (CGFloat)unit.value;
}

/**
 * Creates a UIBezierPath for a circle shape.
 */
static UIBezierPath *_Nullable RCTCreateCirclePath(const CircleShape &circle, CGRect bounds)
{
  CGFloat radius = RCTResolveValueUnit(circle.r, (bounds.size.width + bounds.size.height) / 2.0f);
  CGFloat cx = circle.cx.has_value() ? RCTResolveValueUnit(circle.cx.value(), bounds.size.width)
                                     : bounds.size.width / 2.0f;
  CGFloat cy = circle.cy.has_value() ? RCTResolveValueUnit(circle.cy.value(), bounds.size.height)
                                     : bounds.size.height / 2.0f;

  CGFloat diameter = radius * 2.0f;
  CGRect circleRect = CGRectMake(cx - radius, cy - radius, diameter, diameter);

  return [UIBezierPath bezierPathWithOvalInRect:circleRect];
}

/**
 * Creates a UIBezierPath for an ellipse shape.
 */
static UIBezierPath *_Nullable RCTCreateEllipsePath(const EllipseShape &ellipse, CGRect bounds)
{
  CGFloat rx = RCTResolveValueUnit(ellipse.rx, bounds.size.width);
  CGFloat ry = RCTResolveValueUnit(ellipse.ry, bounds.size.height);
  CGFloat cx = ellipse.cx.has_value() ? RCTResolveValueUnit(ellipse.cx.value(), bounds.size.width)
                                      : bounds.size.width / 2.0f;
  CGFloat cy = ellipse.cy.has_value() ? RCTResolveValueUnit(ellipse.cy.value(), bounds.size.height)
                                      : bounds.size.height / 2.0f;

  CGFloat diameterX = rx * 2.0f;
  CGFloat diameterY = ry * 2.0f;
  CGRect ellipseRect = CGRectMake(cx - rx, cy - ry, diameterX, diameterY);

  return [UIBezierPath bezierPathWithOvalInRect:ellipseRect];
}

/**
 * Creates a UIBezierPath for an inset shape.
 */
static UIBezierPath *_Nullable RCTCreateInsetPath(const InsetShape &inset, CGRect bounds)
{
  CGFloat top = RCTResolveValueUnit(inset.top, bounds.size.height);
  CGFloat right = RCTResolveValueUnit(inset.right, bounds.size.width);
  CGFloat bottom = RCTResolveValueUnit(inset.bottom, bounds.size.height);
  CGFloat left = RCTResolveValueUnit(inset.left, bounds.size.width);

  CGRect insetRect = CGRectMake(
      left, top, bounds.size.width - left - right, bounds.size.height - top - bottom);

  // Return nil if the inset results in an invalid rect
  if (insetRect.size.width < 0 || insetRect.size.height < 0) {
    return nil;
  }

  return [UIBezierPath bezierPathWithRect:insetRect];
}

/**
 * Creates a UIBezierPath for a polygon shape.
 */
static UIBezierPath *_Nullable RCTCreatePolygonPath(const PolygonShape &polygon, CGRect bounds)
{
  if (polygon.points.empty()) {
    return nil;
  }

  UIBezierPath *path = [UIBezierPath bezierPath];

  const auto &firstPoint = polygon.points[0];
  [path moveToPoint:CGPointMake(
                        RCTResolveValueUnit(firstPoint.first, bounds.size.width),
                        RCTResolveValueUnit(firstPoint.second, bounds.size.height))];

  for (size_t i = 1; i < polygon.points.size(); i++) {
    const auto &point = polygon.points[i];
    [path addLineToPoint:CGPointMake(
                             RCTResolveValueUnit(point.first, bounds.size.width),
                             RCTResolveValueUnit(point.second, bounds.size.height))];
  }

  [path closePath];
  return path;
}

/**
 * Creates a UIBezierPath for a rect shape.
 * Note: CSS rect() uses distances from edges (top, right, bottom, left).
 */
static UIBezierPath *_Nullable RCTCreateRectPath(const RectShape &rect, CGRect bounds)
{
  CGFloat top = RCTResolveValueUnit(rect.top, bounds.size.height);
  CGFloat right = RCTResolveValueUnit(rect.right, bounds.size.width);
  CGFloat bottom = RCTResolveValueUnit(rect.bottom, bounds.size.height);
  CGFloat left = RCTResolveValueUnit(rect.left, bounds.size.width);

  // CSS rect() interprets values as distances from edges, creating a clipping rectangle
  CGRect clipRect = CGRectMake(left, top, right - left, bottom - top);

  // Return nil if the rect is invalid
  if (clipRect.size.width < 0 || clipRect.size.height < 0) {
    return nil;
  }

  return [UIBezierPath bezierPathWithRect:clipRect];
}

/**
 * Creates a UIBezierPath for an xywh shape.
 */
static UIBezierPath *_Nullable RCTCreateXywhPath(const XywhShape &xywh, CGRect bounds)
{
  CGFloat x = RCTResolveValueUnit(xywh.x, bounds.size.width);
  CGFloat y = RCTResolveValueUnit(xywh.y, bounds.size.height);
  CGFloat width = RCTResolveValueUnit(xywh.width, bounds.size.width);
  CGFloat height = RCTResolveValueUnit(xywh.height, bounds.size.height);

  CGRect xywhRect = CGRectMake(x, y, width, height);

  // Return nil if the rect is invalid
  if (xywhRect.size.width < 0 || xywhRect.size.height < 0) {
    return nil;
  }

  return [UIBezierPath bezierPathWithRect:xywhRect];
}

/**
 * Creates a UIBezierPath for an SVG path shape.
 * Note: This is a placeholder implementation. Full SVG path parsing would require
 * a complete path data parser supporting all SVG path commands (M, L, C, Q, A, etc.).
 */
static UIBezierPath *_Nullable RCTCreatePathFromSVG(const PathShape &pathShape, CGRect bounds)
{
  // TODO: Implement full SVG path parsing
  // For now, return an empty path to prevent crashes
  // A production implementation would parse pathShape.pathData and convert it to UIBezierPath
  return nil;
}

/**
 * Creates a UIBezierPath from a BasicShape variant.
 */
static UIBezierPath *_Nullable RCTCreatePathFromBasicShape(const BasicShape &basicShape, CGRect bounds)
{
  if (std::holds_alternative<CircleShape>(basicShape)) {
    return RCTCreateCirclePath(std::get<CircleShape>(basicShape), bounds);
  } else if (std::holds_alternative<EllipseShape>(basicShape)) {
    return RCTCreateEllipsePath(std::get<EllipseShape>(basicShape), bounds);
  } else if (std::holds_alternative<InsetShape>(basicShape)) {
    return RCTCreateInsetPath(std::get<InsetShape>(basicShape), bounds);
  } else if (std::holds_alternative<PolygonShape>(basicShape)) {
    return RCTCreatePolygonPath(std::get<PolygonShape>(basicShape), bounds);
  } else if (std::holds_alternative<RectShape>(basicShape)) {
    return RCTCreateRectPath(std::get<RectShape>(basicShape), bounds);
  } else if (std::holds_alternative<XywhShape>(basicShape)) {
    return RCTCreateXywhPath(std::get<XywhShape>(basicShape), bounds);
  } else if (std::holds_alternative<PathShape>(basicShape)) {
    return RCTCreatePathFromSVG(std::get<PathShape>(basicShape), bounds);
  }

  return nil;
}

CAShapeLayer *_Nullable RCTClipPathCreateMaskLayer(const ClipPath &clipPath, CGRect bounds)
{
  // If no shape is specified, no clipping should be applied
  if (!clipPath.shape.has_value()) {
    return nil;
  }

  UIBezierPath *path = RCTCreatePathFromBasicShape(clipPath.shape.value(), bounds);

  // If path creation failed or resulted in an invalid path, return nil
  if (path == nil) {
    return nil;
  }

  CAShapeLayer *maskLayer = [CAShapeLayer layer];
  maskLayer.path = path.CGPath;

  return maskLayer;
}
