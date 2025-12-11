/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.graphics.Bitmap
import android.graphics.BitmapShader
import android.graphics.Canvas
import android.graphics.ColorFilter
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.Shader
import android.graphics.Shader.TileMode
import android.graphics.drawable.Drawable
import android.widget.ImageView
import com.facebook.common.references.CloseableReference
import com.facebook.drawee.drawable.ScalingUtils
import com.facebook.drawee.generic.GenericDraweeHierarchy
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder
import com.facebook.drawee.view.DraweeHolder
import com.facebook.imagepipeline.bitmaps.PlatformBitmapFactory
import com.facebook.imagepipeline.request.BasePostprocessor
import com.facebook.react.uimanager.LengthPercentageType
import com.facebook.react.uimanager.PixelUtil.dpToPx
import com.facebook.react.uimanager.PixelUtil.pxToDp
import com.facebook.react.uimanager.style.BackgroundPosition
import com.facebook.react.uimanager.style.BackgroundRepeat
import com.facebook.react.uimanager.style.BackgroundRepeatKeyword
import com.facebook.react.uimanager.style.BackgroundSize
import com.facebook.react.views.image.ReactImageView

/**
 * A Drawable that uses DraweeHolder to load and display an image mask.
 * This avoids creating Bitmap copies and leverages Fresco's caching and lifecycle management.
 *
 * The mask is applied using Porter-Duff DST_IN mode - the Drawable's alpha channel
 * determines what parts of the view are visible.
 */
internal class ImageMaskDrawable(
    resources: android.content.res.Resources,
) : Drawable() {

    internal val tileProcessor: TilePostprocessor = TilePostprocessor()

    var size: BackgroundSize? = null
        set(value) {
            if (field != value) {
                field = value
                updateTransform()
                invalidateSelf()
            }
        }

    var position: BackgroundPosition? = null
        set(value) {
            if (field != value) {
                field = value
                updateTransform()
                invalidateSelf()
            }
        }

    var repeat: BackgroundRepeat? = null
        set(value) {
            if (field != value) {
                field = value
                updateTransform()
                invalidateSelf()
            }
        }

    private val draweeHolder: DraweeHolder<GenericDraweeHierarchy> =
        DraweeHolder(GenericDraweeHierarchyBuilder.newInstance(resources)
          .setFadeDuration(0)
          .setActualImageScaleType(::createTransformMatrix)
          .build())

    private fun createTransformMatrix(
        out: Matrix,
        parent: Rect,
        childW: Int,
        childH: Int,
        focusX: Float,
        focusY: Float
    ): Matrix {
        val parentW = parent.width().toFloat()
        val parentH = parent.height().toFloat()
        
        if (parentW <= 0 || parentH <= 0 || childW <= 0 || childH <= 0) {
            out.reset()
            return out
        }

        val repeat = this.repeat ?: BackgroundRepeat(BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Repeat)
        val isNoRepeat = repeat.x == BackgroundRepeatKeyword.NoRepeat && repeat.y == BackgroundRepeatKeyword.NoRepeat
        
        out.reset()
        
        if (isNoRepeat) {
            // For no-repeat, this transform handles sizing and positioning
            val (finalWidth, finalHeight) = calculateImageSize(parentW, parentH, childW.toFloat(), childH.toFloat())
            
            val scaleX = finalWidth / childW
            val scaleY = finalHeight / childH
            out.setScale(scaleX, scaleY)
            
            val (translateX, translateY) = calculatePosition(parentW, parentH, finalWidth, finalHeight)
            out.postTranslate(translateX, translateY)
        }
        // For repeat cases, TilePostprocessor handles sizing and tiling,
        // so we use identity matrix (no transform)
        
        return out
    }

    private fun calculateImageSize(
        containerWidth: Float,
        containerHeight: Float,
        imageWidth: Float,
        imageHeight: Float
    ): Pair<Float, Float> {
        var finalWidth = imageWidth
        var finalHeight = imageHeight

        if (size is BackgroundSize.LengthPercentageAuto) {
            val lengthPercentage = (size as BackgroundSize.LengthPercentageAuto).lengthPercentage
            val w = lengthPercentage.x
            val h = lengthPercentage.y
            if (w != null && h != null) {
                finalWidth = positionToPixels(w, containerWidth)
                finalHeight = positionToPixels(h, containerHeight)
            } else if (w != null) {
                finalWidth = positionToPixels(w, containerWidth)
                finalHeight = (finalWidth / imageWidth) * imageHeight
            } else if (h != null) {
                finalHeight = positionToPixels(h, containerHeight)
                finalWidth = (finalHeight / imageHeight) * imageWidth
            }
        }

        return finalWidth to finalHeight
    }

    private fun calculatePosition(
        containerWidth: Float,
        containerHeight: Float,
        imageWidth: Float,
        imageHeight: Float
    ): Pair<Float, Float> {
        val availableSpaceX = containerWidth - imageWidth
        val availableSpaceY = containerHeight - imageHeight

        val translateX = when {
            position?.left != null -> positionToPixels(position!!.left!!, availableSpaceX)
            position?.right != null -> availableSpaceX - positionToPixels(position!!.right!!, availableSpaceX)
            else -> 0.0f
        }

        val translateY = when {
            position?.top != null -> positionToPixels(position!!.top!!, availableSpaceY)
            position?.bottom != null -> availableSpaceY - positionToPixels(position!!.bottom!!, availableSpaceY)
            else -> 0.0f
        }

        return translateX to translateY
    }

    private fun positionToPixels(lengthPercentage: com.facebook.react.uimanager.LengthPercentage, availableSpace: Float): Float =
        if (lengthPercentage.type == LengthPercentageType.PERCENT) {
            lengthPercentage.resolve(availableSpace)
        } else {
            lengthPercentage.resolve(availableSpace).dpToPx()
        }

    private fun updateTransform() {
        // Force update by invalidating the hierarchy
        draweeHolder.topLevelDrawable?.invalidateSelf()
        invalidateSelf()

        // For repeat cases, need to reload image to trigger tileProcessor reprocessing with new bounds
        // For no-repeat cases, the transformer will be called automatically by Drawee with new bounds
        val controller = draweeHolder.controller
        if (controller != null) {
            val isRepeat = repeat?.x != BackgroundRepeatKeyword.NoRepeat || repeat?.y != BackgroundRepeatKeyword.NoRepeat
            if (isRepeat) {
                // Reload to trigger tileProcessor with updated bounds
//                draweeHolder.controller = controller
            } else {
                // For no-repeat, just invalidate - transformer will be called with new bounds automatically
//                invalidateSelf()
            }
        }
    }

    /**
     * Sets the ImageRequest to load as the mask.
     * The DraweeHolder will handle loading, caching, and lifecycle management.
     */
    fun setImageRequest(imageRequest: com.facebook.imagepipeline.request.ImageRequest?) {
        draweeHolder.controller = imageRequest?.let {
            com.facebook.drawee.backends.pipeline.Fresco.newDraweeControllerBuilder()
                .setImageRequest(it)
                .setAutoPlayAnimations(true)
                .setOldController(draweeHolder.controller)
                .build()
        }
    }

    fun onAttach() = draweeHolder.onAttach()

    fun onDetach() = draweeHolder.onDetach()

    override fun draw(canvas: Canvas) {
        val drawable = draweeHolder.topLevelDrawable ?: return

        // Set bounds if not already set
        if (drawable.bounds.isEmpty) {
            drawable.bounds = bounds
        }

        drawable.draw(canvas)
    }

    /**
     * Draws this Drawable with Porter-Duff compositing applied.
     * This is called from ReactViewGroup.draw() to apply the mask.
     *
     * Since we're already inside a saveLayer in ReactViewGroup.draw(), we create
     * a nested saveLayer with Porter-Duff Paint. When we restore this inner layer,
     * it composites with the outer layer using Porter-Duff DST_IN mode.
     *
     * This avoids creating Bitmap copies - the DraweeDrawable is drawn directly.
     */
    fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
        val drawable = draweeHolder.topLevelDrawable ?: return
        val bounds = bounds.takeUnless { it.isEmpty } ?: return

        // Set bounds if not already set
        if (drawable.bounds.isEmpty) {
            drawable.bounds = bounds
        }

        // Create a nested saveLayer with Porter-Duff DST_IN mode
        // When we restore this layer, it will composite with the outer layer (which contains
        // the background + children) using DST_IN mode, effectively masking the content
        val saveCount = canvas.saveLayer(
            bounds.left.toFloat(),
            bounds.top.toFloat(),
            bounds.right.toFloat(),
            bounds.bottom.toFloat(),
            maskPaint
        )

        // Draw the DraweeDrawable to the inner layer
        drawable.draw(canvas)

        // Restore the inner layer - this composites it with the outer layer using DST_IN mode
        canvas.restoreToCount(saveCount)
    }

    override fun setBounds(left: Int, top: Int, right: Int, bottom: Int) {
        val changed = bounds.left != left || bounds.top != top || 
                      bounds.right != right || bounds.bottom != bottom
        super.setBounds(left, top, right, bottom)
        draweeHolder.topLevelDrawable?.setBounds(left, top, right, bottom)
        
        // Update transforms when bounds change
        if (changed) {
            updateTransform()
        }
    }

    override fun setAlpha(alpha: Int) {
        draweeHolder.topLevelDrawable?.alpha = alpha
        invalidateSelf()
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        draweeHolder.topLevelDrawable?.colorFilter = colorFilter
        invalidateSelf()
    }

    @Deprecated("Deprecated in Java")
    override fun getOpacity(): Int = draweeHolder.topLevelDrawable?.opacity ?: PixelFormat.TRANSLUCENT

  internal inner class TilePostprocessor : BasePostprocessor() {
    override fun process(
      source: Bitmap,
      bitmapFactory: PlatformBitmapFactory,
    ): CloseableReference<Bitmap> {
      val containerWidth = 1050
      val containerHeight = 1050

      // Guard against invalid dimensions
      if (containerWidth <= 0 || containerHeight <= 0 || source.width <= 0 || source.height <= 0) {
        return super.process(source, bitmapFactory)
      }

      val repeat = this@ImageMaskDrawable.repeat
        ?: BackgroundRepeat(BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Repeat)

      // Calculate tile size based on size property
      var (tileWidth, tileHeight) = calculateImageSize(
        containerWidth.toFloat(),
        containerHeight.toFloat(),
        source.width.toFloat(),
        source.height.toFloat()
      )

      // Adjust tile size for Round mode - tiles are scaled to fit evenly
      if (repeat.x == BackgroundRepeatKeyword.Round && tileWidth > 0) {
        val numRepeats = kotlin.math.round(containerWidth / tileWidth)
        if (numRepeats > 0) {
          tileWidth = containerWidth / numRepeats
        }
      }
      if (repeat.y == BackgroundRepeatKeyword.Round && tileHeight > 0) {
        val numRepeats = kotlin.math.round(containerHeight / tileHeight)
        if (numRepeats > 0) {
          tileHeight = containerHeight / numRepeats
        }
      }

      if (tileWidth <= 0 || tileHeight <= 0) {
        return super.process(source, bitmapFactory)
      }

      val output = bitmapFactory.createBitmap(containerWidth, containerHeight)
      try {
        val canvas = Canvas(output.get())

        // Create scaled tile bitmap
        val tileWidthInt = tileWidth.toInt().coerceAtLeast(1)
        val tileHeightInt = tileHeight.toInt().coerceAtLeast(1)
        val scaledTile = Bitmap.createScaledBitmap(source, tileWidthInt, tileHeightInt, true)

        try {
          // Space mode requires manual tile drawing with spacing
          val needsManualTiling = repeat.x == BackgroundRepeatKeyword.Space ||
            repeat.y == BackgroundRepeatKeyword.Space

          if (needsManualTiling) {
            drawTilesManually(canvas, scaledTile, containerWidth, containerHeight,
              tileWidth, tileHeight, repeat)
          } else {
            drawWithShader(canvas, scaledTile, containerWidth, containerHeight,
              tileWidth, tileHeight, repeat)
          }
        } finally {
          scaledTile.recycle()
        }

        return output.clone()
      } finally {
        CloseableReference.closeSafely(output)
      }
    }

    private fun drawWithShader(
      canvas: Canvas,
      scaledTile: Bitmap,
      containerWidth: Int,
      containerHeight: Int,
      tileWidth: Float,
      tileHeight: Float,
      repeat: BackgroundRepeat
    ) {
      val tileModeX = when (repeat.x) {
        BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Round -> TileMode.REPEAT
        else -> TileMode.CLAMP
      }
      val tileModeY = when (repeat.y) {
        BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Round -> TileMode.REPEAT
        else -> TileMode.CLAMP
      }

      val shader = BitmapShader(scaledTile, tileModeX, tileModeY)

      // Calculate position offset for tile placement
      val (translateX, translateY) = calculatePosition(
        containerWidth.toFloat(),
        containerHeight.toFloat(),
        tileWidth,
        tileHeight
      )

      tileMatrix.reset()
      tileMatrix.setTranslate(translateX, translateY)
      shader.setLocalMatrix(tileMatrix)

      val paint = Paint().apply {
        isAntiAlias = true
        setShader(shader)
      }

      canvas.drawRect(0f, 0f, containerWidth.toFloat(), containerHeight.toFloat(), paint)
    }

    private fun drawTilesManually(
      canvas: Canvas,
      scaledTile: Bitmap,
      containerWidth: Int,
      containerHeight: Int,
      tileWidth: Float,
      tileHeight: Float,
      repeat: BackgroundRepeat
    ) {
      val (initialX, initialY) = calculatePosition(
        containerWidth.toFloat(),
        containerHeight.toFloat(),
        tileWidth,
        tileHeight
      )

      // Calculate X tiles count and spacing
      var xTilesCount = 1
      var xSpacing = 0f
      var startX = initialX

      when (repeat.x) {
        BackgroundRepeatKeyword.Space -> {
          // Space: images are pinned to edges with even spacing between them
          val widthOfEdgePinnedImages = tileWidth * 2
          val availableWidthForCenterImages = containerWidth - widthOfEdgePinnedImages
          val roundedTileWidth = kotlin.math.round(tileWidth)
          if (roundedTileWidth > 0 && availableWidthForCenterImages >= 0) {
            val centerImagesCount = kotlin.math.floor(availableWidthForCenterImages / roundedTileWidth).toInt()
            val centerImagesWidth = centerImagesCount * tileWidth
            val totalFreeSpace = availableWidthForCenterImages - centerImagesWidth
            val totalInstances = centerImagesCount + 2
            xSpacing = totalFreeSpace / (totalInstances - 1)
            xTilesCount = totalInstances
            startX = 0f
          }
        }
        BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Round -> {
          val roundedTileWidth = kotlin.math.round(tileWidth)
          if (roundedTileWidth > 0) {
            val tilesBeforeX = kotlin.math.ceil(kotlin.math.round(startX) / roundedTileWidth).toInt()
            val tilesAfterX = kotlin.math.ceil(kotlin.math.round(containerWidth - startX) / roundedTileWidth).toInt()
            xTilesCount = tilesBeforeX + tilesAfterX
            startX -= tilesBeforeX * tileWidth
          }
        }
        else -> { /* NoRepeat - single tile at position */ }
      }

      // Calculate Y tiles count and spacing
      var yTilesCount = 1
      var ySpacing = 0f
      var startY = initialY

      when (repeat.y) {
        BackgroundRepeatKeyword.Space -> {
          val heightOfEdgePinnedImages = tileHeight * 2
          val availableHeightForCenterImages = containerHeight - heightOfEdgePinnedImages
          val roundedTileHeight = kotlin.math.round(tileHeight)
          if (roundedTileHeight > 0 && availableHeightForCenterImages >= 0) {
            val centerImagesCount = kotlin.math.floor(availableHeightForCenterImages / roundedTileHeight).toInt()
            val centerImagesHeight = centerImagesCount * tileHeight
            val totalFreeSpace = availableHeightForCenterImages - centerImagesHeight
            val totalInstances = centerImagesCount + 2
            ySpacing = totalFreeSpace / (totalInstances - 1)
            yTilesCount = totalInstances
            startY = 0f
          }
        }
        BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Round -> {
          val roundedTileHeight = kotlin.math.round(tileHeight)
          if (roundedTileHeight > 0) {
            val tilesBeforeY = kotlin.math.ceil(kotlin.math.round(startY) / roundedTileHeight).toInt()
            val tilesAfterY = kotlin.math.ceil(kotlin.math.round(containerHeight - startY) / roundedTileHeight).toInt()
            yTilesCount = tilesBeforeY + tilesAfterY
            startY -= tilesBeforeY * tileHeight
          }
        }
        else -> { /* NoRepeat - single tile at position */ }
      }

      // Draw tiles
      var translateX = startX
      for (i in 0 until xTilesCount) {
        var translateY = startY
        for (j in 0 until yTilesCount) {
          canvas.drawBitmap(scaledTile, translateX, translateY, null)
          translateY += tileHeight + ySpacing
        }
        translateX += tileWidth + xSpacing
      }
    }
  }

   companion object {
    private val tileMatrix = Matrix()
   }
}


