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
import com.facebook.react.uimanager.PixelUtil

/** Represents a value with a unit type (pixel, percent, etc.) */
public data class ValueUnit(
    val value: Float,
    val unit: UnitType,
) {
  public companion object {
    public fun parse(map: ReadableMap, key: String): ValueUnit? {
      if (!map.hasKey(key)) return null
      val valueMap = map.getMap(key) ?: return null
      val value = valueMap.getDouble("value").toFloat()
      val unit = valueMap.getString("unit") ?: return null
      return ValueUnit(value, UnitType.fromString(unit))
    }
  }
}

/** Unit types for values */
public enum class UnitType {
  Point,
  Percent;

  public companion object {
    public fun fromString(value: String): UnitType {
      return when (value.lowercase()) {
        "point" -> Point
        "percent" -> Percent
        else -> Point
      }
    }
  }
}

/** Circle shape: circle(radius at cx cy) */
public data class CircleShape(
    val r: ValueUnit,
    val cx: ValueUnit? = null,
    val cy: ValueUnit? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): CircleShape? {
      val r = ValueUnit.parse(map, "r") ?: return null
      val cx = ValueUnit.parse(map, "cx")
      val cy = ValueUnit.parse(map, "cy")
      return CircleShape(r, cx, cy)
    }
  }
}

/** Ellipse shape: ellipse(rx ry at cx cy) */
public data class EllipseShape(
    val rx: ValueUnit,
    val ry: ValueUnit,
    val cx: ValueUnit? = null,
    val cy: ValueUnit? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): EllipseShape? {
      val rx = ValueUnit.parse(map, "rx") ?: return null
      val ry = ValueUnit.parse(map, "ry") ?: return null
      val cx = ValueUnit.parse(map, "cx")
      val cy = ValueUnit.parse(map, "cy")
      return EllipseShape(rx, ry, cx, cy)
    }
  }
}

/** Inset shape: inset(top right bottom left round radius) */
public data class InsetShape(
    val top: ValueUnit,
    val right: ValueUnit,
    val bottom: ValueUnit,
    val left: ValueUnit,
    val borderRadius: ValueUnit? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): InsetShape? {
      val top = ValueUnit.parse(map, "top") ?: return null
      val right = ValueUnit.parse(map, "right") ?: return null
      val bottom = ValueUnit.parse(map, "bottom") ?: return null
      val left = ValueUnit.parse(map, "left") ?: return null
      val borderRadius = ValueUnit.parse(map, "borderRadius")
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
    val points: List<Pair<ValueUnit, ValueUnit>>,
    val fillRule: FillRule? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): PolygonShape? {
      if (!map.hasKey("points")) return null
      val pointsArray = map.getArray("points") ?: return null
      val points = mutableListOf<Pair<ValueUnit, ValueUnit>>()

      for (i in 0 until pointsArray.size()) {
        val pointMap = pointsArray.getMap(i) ?: continue
        val x = ValueUnit.parse(pointMap, "x") ?: continue
        val y = ValueUnit.parse(pointMap, "y") ?: continue
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
    val top: ValueUnit,
    val right: ValueUnit,
    val bottom: ValueUnit,
    val left: ValueUnit,
    val borderRadius: ValueUnit? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): RectShape? {
      val top = ValueUnit.parse(map, "top") ?: return null
      val right = ValueUnit.parse(map, "right") ?: return null
      val bottom = ValueUnit.parse(map, "bottom") ?: return null
      val left = ValueUnit.parse(map, "left") ?: return null
      val borderRadius = ValueUnit.parse(map, "borderRadius")
      return RectShape(top, right, bottom, left, borderRadius)
    }
  }
}

/** Xywh shape: xywh(x y width height round radius) */
public data class XywhShape(
    val x: ValueUnit,
    val y: ValueUnit,
    val width: ValueUnit,
    val height: ValueUnit,
    val borderRadius: ValueUnit? = null,
) {
  public companion object {
    public fun parse(map: ReadableMap): XywhShape? {
      val x = ValueUnit.parse(map, "x") ?: return null
      val y = ValueUnit.parse(map, "y") ?: return null
      val width = ValueUnit.parse(map, "width") ?: return null
      val height = ValueUnit.parse(map, "height") ?: return null
      val borderRadius = ValueUnit.parse(map, "borderRadius")
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
