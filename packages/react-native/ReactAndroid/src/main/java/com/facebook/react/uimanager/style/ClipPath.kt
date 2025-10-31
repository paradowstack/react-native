/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.style

import android.content.Context
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.facebook.react.uimanager.LengthPercentage

/** Circle shape: circle(radius at cx cy) */
public data class CircleShape(
    val r: LengthPercentage,
    val cx: LengthPercentage? = null,
    val cy: LengthPercentage? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): CircleShape? {
      val r = if (map.hasKey("r")) LengthPercentage.setFromDynamic(map.getDynamic("r")) else null
      if (r == null) return null
      val cx = if (map.hasKey("cx")) LengthPercentage.setFromDynamic(map.getDynamic("cx")) else null
      val cy = if (map.hasKey("cy")) LengthPercentage.setFromDynamic(map.getDynamic("cy")) else null
      return CircleShape(r, cx, cy)
    }
  }
}

/** Ellipse shape: ellipse(rx ry at cx cy) */
public data class EllipseShape(
    val rx: LengthPercentage,
    val ry: LengthPercentage,
    val cx: LengthPercentage? = null,
    val cy: LengthPercentage? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): EllipseShape? {
      val rx = if (map.hasKey("rx")) LengthPercentage.setFromDynamic(map.getDynamic("rx")) else null
      val ry = if (map.hasKey("ry")) LengthPercentage.setFromDynamic(map.getDynamic("ry")) else null
      if (rx == null || ry == null) return null
      val cx = if (map.hasKey("cx")) LengthPercentage.setFromDynamic(map.getDynamic("cx")) else null
      val cy = if (map.hasKey("cy")) LengthPercentage.setFromDynamic(map.getDynamic("cy")) else null
      return EllipseShape(rx, ry, cx, cy)
    }
  }
}

/** Inset shape: inset(top right bottom left round radius) */
public data class InsetShape(
    val top: LengthPercentage,
    val right: LengthPercentage,
    val bottom: LengthPercentage,
    val left: LengthPercentage,
    val borderRadius: LengthPercentage? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): InsetShape? {
      val top = if (map.hasKey("top")) LengthPercentage.setFromDynamic(map.getDynamic("top")) else null
      val right = if (map.hasKey("right")) LengthPercentage.setFromDynamic(map.getDynamic("right")) else null
      val bottom = if (map.hasKey("bottom")) LengthPercentage.setFromDynamic(map.getDynamic("bottom")) else null
      val left = if (map.hasKey("left")) LengthPercentage.setFromDynamic(map.getDynamic("left")) else null
      if (top == null || right == null || bottom == null || left == null) return null
      val borderRadius = if (map.hasKey("borderRadius")) LengthPercentage.setFromDynamic(map.getDynamic("borderRadius")) else null
      return InsetShape(top, right, bottom, left, borderRadius)
    }
  }
}

/** Fill rule for polygon */
public enum class FillRule {
  NonZero,
  EvenOdd;

  public companion object {
    public fun fromString(value: String): FillRule {
      return when (value.lowercase()) {
        "nonzero" -> NonZero
        "evenodd" -> EvenOdd
        else -> NonZero
      }
    }
  }
}

/** Polygon shape: polygon(fillRule, x1 y1, x2 y2, ...) */
public data class PolygonShape(
    val points: List<Pair<LengthPercentage, LengthPercentage>>,
    val fillRule: FillRule? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): PolygonShape? {
      if (!map.hasKey("points")) return null
      val pointsArray = map.getArray("points") ?: return null
      val points = mutableListOf<Pair<LengthPercentage, LengthPercentage>>()

      for (i in 0 until pointsArray.size()) {
        val pointMap = pointsArray.getMap(i) ?: continue
        val x = if (pointMap.hasKey("x")) LengthPercentage.setFromDynamic(pointMap.getDynamic("x")) else null
        val y = if (pointMap.hasKey("y")) LengthPercentage.setFromDynamic(pointMap.getDynamic("y")) else null
        if (x == null || y == null) continue
        points.add(Pair(x, y))
      }

      val fillRule =
          if (map.hasKey("fillRule")) {
            FillRule.fromString(map.getString("fillRule") ?: "nonzero")
          } else {
            null
          }

      return PolygonShape(points, fillRule)
    }
  }
}

/** Rect shape: rect(top right bottom left round radius) */
public data class RectShape(
    val top: LengthPercentage,
    val right: LengthPercentage,
    val bottom: LengthPercentage,
    val left: LengthPercentage,
    val borderRadius: LengthPercentage? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): RectShape? {
      val top = if (map.hasKey("top")) LengthPercentage.setFromDynamic(map.getDynamic("top")) else null
      val right = if (map.hasKey("right")) LengthPercentage.setFromDynamic(map.getDynamic("right")) else null
      val bottom = if (map.hasKey("bottom")) LengthPercentage.setFromDynamic(map.getDynamic("bottom")) else null
      val left = if (map.hasKey("left")) LengthPercentage.setFromDynamic(map.getDynamic("left")) else null
      if (top == null || right == null || bottom == null || left == null) return null
      val borderRadius = if (map.hasKey("borderRadius")) LengthPercentage.setFromDynamic(map.getDynamic("borderRadius")) else null
      return RectShape(top, right, bottom, left, borderRadius)
    }
  }
}

/** Xywh shape: xywh(x y width height round radius) */
public data class XywhShape(
    val x: LengthPercentage,
    val y: LengthPercentage,
    val width: LengthPercentage,
    val height: LengthPercentage,
    val borderRadius: LengthPercentage? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): XywhShape? {
      val x = if (map.hasKey("x")) LengthPercentage.setFromDynamic(map.getDynamic("x")) else null
      val y = if (map.hasKey("y")) LengthPercentage.setFromDynamic(map.getDynamic("y")) else null
      val width = if (map.hasKey("width")) LengthPercentage.setFromDynamic(map.getDynamic("width")) else null
      val height = if (map.hasKey("height")) LengthPercentage.setFromDynamic(map.getDynamic("height")) else null
      if (x == null || y == null || width == null || height == null) return null
      val borderRadius = if (map.hasKey("borderRadius")) LengthPercentage.setFromDynamic(map.getDynamic("borderRadius")) else null
      return XywhShape(x, y, width, height, borderRadius)
    }
  }
}

/** Basic shape variant */
public sealed class BasicShape {
  public data class Circle(val shape: CircleShape) : BasicShape()

  public data class Ellipse(val shape: EllipseShape) : BasicShape()

  public data class Inset(val shape: InsetShape) : BasicShape()

  public data class Polygon(val shape: PolygonShape) : BasicShape()

  public data class Rect(val shape: RectShape) : BasicShape()

  public data class Xywh(val shape: XywhShape) : BasicShape()

  public companion object {
    public fun parse(map: ReadableMap): BasicShape? {
      if (!map.hasKey("type")) return null
      val type = map.getString("type") ?: return null

      return when (type.lowercase()) {
        "circle" -> {
          val circle = CircleShape.parse(map) ?: return null
          Circle(circle)
        }
        "ellipse" -> {
          val ellipse = EllipseShape.parse(map) ?: return null
          Ellipse(ellipse)
        }
        "inset" -> {
          val inset = InsetShape.parse(map) ?: return null
          Inset(inset)
        }
        "polygon" -> {
          val polygon = PolygonShape.parse(map) ?: return null
          Polygon(polygon)
        }
        "rect" -> {
          val rect = RectShape.parse(map) ?: return null
          Rect(rect)
        }
        "xywh" -> {
          val xywh = XywhShape.parse(map) ?: return null
          Xywh(xywh)
        }
        else -> null
      }
    }
  }
}

/** Geometry box types */
public enum class GeometryBox {
  MarginBox,
  BorderBox,
  ContentBox,
  PaddingBox,
  FillBox,
  StrokeBox,
  ViewBox;

  public companion object {
    public fun fromString(value: String): GeometryBox {
      return when (value.lowercase()) {
        "margin-box" -> MarginBox
        "border-box" -> BorderBox
        "content-box" -> ContentBox
        "padding-box" -> PaddingBox
        "fill-box" -> FillBox
        "stroke-box" -> StrokeBox
        "view-box" -> ViewBox
        else -> BorderBox
      }
    }
  }
}

/** ClipPath property value */
public data class ClipPath(
    val shape: BasicShape? = null,
    val geometryBox: GeometryBox? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap?, context: Context): ClipPath? {
      if (map == null) return null

      val shape =
          if (map.hasKey("shape") && map.getType("shape") == ReadableType.Map) {
            val shapeMap = map.getMap("shape")
            if (shapeMap != null) BasicShape.parse(shapeMap) else null
          } else {
            null
          }

      val geometryBox =
          if (map.hasKey("geometryBox")) {
            GeometryBox.fromString(map.getString("geometryBox") ?: "border-box")
          } else {
            null
          }

      return ClipPath(shape, geometryBox)
    }
  }
}
