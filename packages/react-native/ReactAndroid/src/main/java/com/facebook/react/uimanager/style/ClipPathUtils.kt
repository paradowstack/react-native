/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.style

import android.graphics.Path
import android.graphics.RectF
import android.graphics.Path.FillType
import com.facebook.react.uimanager.LengthPercentage
import com.facebook.react.uimanager.LengthPercentageType
import com.facebook.react.uimanager.PixelUtil

/** Utility class for converting ClipPath data structures to Android Path objects */
public object ClipPathUtils {

  /**
   * Resolves a LengthPercentage to a pixel value based on the reference dimension
   *
   * @param lengthPercentage The value and unit to resolve
   * @param referenceDimension The reference size (width or height) for percentage calculations
   * @return The resolved pixel value
   */
  private fun resolveLengthPercentage(lengthPercentage: LengthPercentage, referenceDimension: Float): Float {
    return when (lengthPercentage.type) {
      LengthPercentageType.POINT -> PixelUtil.toPixelFromDIP(lengthPercentage.resolve(1f))
      LengthPercentageType.PERCENT -> lengthPercentage.resolve(referenceDimension)
    }
  }

  /**
   * Creates an Android Path from a BasicShape
   *
   * @param basicShape The basic shape to convert
   * @param bounds The bounding rectangle for the shape
   * @return An Android Path representing the shape
   */
  public fun createPathFromBasicShape(basicShape: BasicShape, bounds: RectF): Path {
    return when (basicShape) {
      is BasicShape.Circle -> createCirclePath(basicShape.shape, bounds)
      is BasicShape.Ellipse -> createEllipsePath(basicShape.shape, bounds)
      is BasicShape.Inset -> createInsetPath(basicShape.shape, bounds)
      is BasicShape.Polygon -> createPolygonPath(basicShape.shape, bounds)
      is BasicShape.Rect -> createRectPath(basicShape.shape, bounds)
      is BasicShape.Xywh -> createXywhPath(basicShape.shape, bounds)
    }
  }

  /**
   * Creates a circular path
   *
   * @param circle The circle shape definition
   * @param bounds The reference bounds
   * @return A Path representing the circle
   */
  private fun createCirclePath(circle: CircleShape, bounds: RectF): Path {
    val path = Path()

    // Resolve radius (use smaller dimension as reference for percentages)
    val referenceDimension = minOf(bounds.width(), bounds.height())
    val radius = resolveLengthPercentage(circle.r, referenceDimension)

    // Resolve center (default to center of bounds)
    val cx =
        if (circle.cx != null) {
          bounds.left + resolveLengthPercentage(circle.cx, bounds.width())
        } else {
          bounds.centerX()
        }

    val cy =
        if (circle.cy != null) {
          bounds.top + resolveLengthPercentage(circle.cy, bounds.height())
        } else {
          bounds.centerY()
        }

    println("[MYDEBUG] Circle center: ($cx, $cy), radius: $radius")
    path.addCircle(cx, cy, radius, Path.Direction.CW)
    return path
  }

  /**
   * Creates an ellipse path
   *
   * @param ellipse The ellipse shape definition
   * @param bounds The reference bounds
   * @return A Path representing the ellipse
   */
  private fun createEllipsePath(ellipse: EllipseShape, bounds: RectF): Path {
    val path = Path()

    // Resolve radii
    val rx = resolveLengthPercentage(ellipse.rx, bounds.width())
    val ry = resolveLengthPercentage(ellipse.ry, bounds.height())

    // Resolve center (default to center of bounds)
    val cx =
        if (ellipse.cx != null) {
          bounds.left + resolveLengthPercentage(ellipse.cx, bounds.width())
        } else {
          bounds.centerX()
        }

    val cy =
        if (ellipse.cy != null) {
          bounds.top + resolveLengthPercentage(ellipse.cy, bounds.height())
        } else {
          bounds.centerY()
        }

    val oval = RectF(cx - rx, cy - ry, cx + rx, cy + ry)
    path.addOval(oval, Path.Direction.CW)
    return path
  }

  /**
   * Creates an inset path (rectangle with insets and optional border radius)
   *
   * @param inset The inset shape definition
   * @param bounds The reference bounds
   * @return A Path representing the inset shape
   */
  private fun createInsetPath(inset: InsetShape, bounds: RectF): Path {
    val path = Path()

    // Calculate inset rectangle
    val top = bounds.top + resolveLengthPercentage(inset.top, bounds.height())
    val right = bounds.right - resolveLengthPercentage(inset.right, bounds.width())
    val bottom = bounds.bottom - resolveLengthPercentage(inset.bottom, bounds.height())
    val left = bounds.left + resolveLengthPercentage(inset.left, bounds.width())

    val rect = RectF(left, top, right, bottom)

    // Add border radius if specified
    if (inset.borderRadius != null) {
      val referenceDimension = minOf(rect.width(), rect.height())
      val radius = resolveLengthPercentage(inset.borderRadius, referenceDimension)
      path.addRoundRect(rect, radius, radius, Path.Direction.CW)
    } else {
      path.addRect(rect, Path.Direction.CW)
    }

    return path
  }

  /**
   * Creates a polygon path
   *
   * @param polygon The polygon shape definition
   * @param bounds The reference bounds
   * @return A Path representing the polygon
   */
  private fun createPolygonPath(polygon: PolygonShape, bounds: RectF): Path {
    val path = Path()

    if (polygon.points.isEmpty()) {
      return path
    }

    // Set fill rule
    when (polygon.fillRule) {
      FillRule.EvenOdd -> path.fillType = FillType.EVEN_ODD
      FillRule.NonZero,
      null -> path.fillType = FillType.WINDING
    }

    // Add points
    val firstPoint = polygon.points[0]
    val firstX = bounds.left + resolveLengthPercentage(firstPoint.first, bounds.width())
    val firstY = bounds.top + resolveLengthPercentage(firstPoint.second, bounds.height())
    path.moveTo(firstX, firstY)

    for (i in 1 until polygon.points.size) {
      val point = polygon.points[i]
      val x = bounds.left + resolveLengthPercentage(point.first, bounds.width())
      val y = bounds.top + resolveLengthPercentage(point.second, bounds.height())
      path.lineTo(x, y)
    }

    path.close()
    return path
  }

  /**
   * Creates a rect path (using edge distances)
   *
   * @param rect The rect shape definition
   * @param bounds The reference bounds
   * @return A Path representing the rect
   */
  private fun createRectPath(rect: RectShape, bounds: RectF): Path {
    val path = Path()

    // CSS rect() uses distances from edges: rect(top, right, bottom, left)
    val top = bounds.top + resolveLengthPercentage(rect.top, bounds.height())
    val right = bounds.left + resolveLengthPercentage(rect.right, bounds.width())
    val bottom = bounds.top + resolveLengthPercentage(rect.bottom, bounds.height())
    val left = bounds.left + resolveLengthPercentage(rect.left, bounds.width())

    val rectF = RectF(left, top, right, bottom)
    println("[MYDEBUG] Rect edges: top=$top, right=$right, bottom=$bottom, left=$left")

    // Add border radius if specified
    if (rect.borderRadius != null) {
      val referenceDimension = minOf(rectF.width(), rectF.height())
      val radius = resolveLengthPercentage(rect.borderRadius, referenceDimension)
      path.addRoundRect(rectF, radius, radius, Path.Direction.CW)
    } else {
      path.addRect(rectF, Path.Direction.CW)
    }

    return path
  }

  /**
   * Creates an xywh path (rectangle with x, y, width, height)
   *
   * @param xywh The xywh shape definition
   * @param bounds The reference bounds
   * @return A Path representing the xywh rectangle
   */
  private fun createXywhPath(xywh: XywhShape, bounds: RectF): Path {
    val path = Path()

    // Calculate rectangle from x, y, width, height
    val x = bounds.left + resolveLengthPercentage(xywh.x, bounds.width())
    val y = bounds.top + resolveLengthPercentage(xywh.y, bounds.height())
    val width = resolveLengthPercentage(xywh.width, bounds.width())
    val height = resolveLengthPercentage(xywh.height, bounds.height())

    val rect = RectF(x, y, x + width, y + height)

    // Add border radius if specified
    if (xywh.borderRadius != null) {
      val referenceDimension = minOf(rect.width(), rect.height())
      val radius = resolveLengthPercentage(xywh.borderRadius, referenceDimension)
      path.addRoundRect(rect, radius, radius, Path.Direction.CW)
    } else {
      path.addRect(rect, Path.Direction.CW)
    }

    return path
  }
}
