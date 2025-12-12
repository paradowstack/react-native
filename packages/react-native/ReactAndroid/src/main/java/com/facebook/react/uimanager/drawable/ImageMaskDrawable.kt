/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.content.Context
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
import android.graphics.drawable.Animatable
import android.graphics.drawable.Drawable
import android.view.View
import com.facebook.common.references.CloseableReference
import com.facebook.drawee.backends.pipeline.Fresco
import com.facebook.drawee.generic.GenericDraweeHierarchy
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder
import com.facebook.drawee.view.DraweeHolder
import com.facebook.imagepipeline.image.ImageInfo
import com.facebook.imagepipeline.bitmaps.PlatformBitmapFactory
import com.facebook.imagepipeline.request.BasePostprocessor
import com.facebook.imagepipeline.request.ImageRequest
import com.facebook.imagepipeline.request.ImageRequestBuilder
import com.facebook.react.modules.fresco.ReactNetworkImageRequest
import com.facebook.react.uimanager.LengthPercentageType
import com.facebook.react.uimanager.PixelUtil.dpToPx
import com.facebook.react.uimanager.style.BackgroundPosition
import com.facebook.react.uimanager.style.BackgroundRepeat
import com.facebook.react.uimanager.style.BackgroundRepeatKeyword
import com.facebook.react.uimanager.style.BackgroundSize
import com.facebook.react.views.image.MultiPostprocessor
import com.facebook.react.views.imagehelper.ImageSource

/**
 * A Drawable that uses DraweeHolder to load and display an image mask.
 * This avoids creating Bitmap copies and leverages Fresco's caching and lifecycle management.
 *
 * The mask is applied using Porter-Duff DST_IN mode - the Drawable's alpha channel
 * determines what parts of the view are visible.
 */
internal class ImageMaskDrawable(
    private val context: Context,
) : Drawable(), Drawable.Callback {

    private val tileProcessor: TilePostprocessor = TilePostprocessor()
    
    private var imageUrl: String? = null
    private var isDirty: Boolean = false

    var size: BackgroundSize? = null
        set(value) {
            if (field != value) {
                field = value
                if (needsTiling()) {
                    isDirty = true
                }
                updateTransform()
                invalidateSelf()
            }
        }

    var position: BackgroundPosition? = null
        set(value) {
            if (field != value) {
                field = value
                if (needsTiling()) {
                    isDirty = true
                }
                updateTransform()
                invalidateSelf()
            }
        }

    var repeat: BackgroundRepeat? = null
        set(value) {
            if (field != value) {
                val wasTiling = needsTiling()
                field = value
                val nowTiling = needsTiling()
                if (wasTiling || nowTiling) {
                    isDirty = true
                }
                updateTransform()
                invalidateSelf()
            }
        }

    private val draweeHolder: DraweeHolder<GenericDraweeHierarchy> =
        DraweeHolder(GenericDraweeHierarchyBuilder.newInstance(context.resources)
          .setFadeDuration(0)
          .setActualImageScaleType(::createTransformMatrix)
          .build()).apply { this.topLevelDrawable?.callback = this@ImageMaskDrawable }
    
    /**
     * Returns true if tiling is needed (any repeat mode other than no-repeat on both axes).
     */
    private fun needsTiling(): Boolean {
        val r = repeat ?: return true // Default is repeat
        return r.x != BackgroundRepeatKeyword.NoRepeat || r.y != BackgroundRepeatKeyword.NoRepeat
    }

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
        
        // For repeat cases, maybeUpdateView will handle reloading with new bounds
        // For no-repeat cases, the transformer will be called automatically by Drawee
        maybeUpdateView()
    }
    
    /**
     * Sets the image URL to load as the mask.
     * The image will be loaded when bounds are available (for tiling) or immediately (for no-repeat).
     */
    fun setImageUrl(url: String) {
        if (imageUrl != url) {
            imageUrl = url
            isDirty = true
            maybeUpdateView()
        }
    }
    
    /**
     * Rebuilds the image request if needed.
     * For tiling modes, waits until valid bounds are available because TilePostprocessor
     * needs the container dimensions. For no-repeat mode, loads immediately.
     */
    private fun maybeUpdateView() {
        if (!isDirty) {
            return
        }

        val hasBounds = bounds.width() > 0 && bounds.height() > 0
        if (!hasBounds) {
          return
        }

        val url = imageUrl ?: return
        
        // For tiling modes, we MUST wait for valid bounds
        // because TilePostprocessor needs the container dimensions
        val imageSource = ImageSource(context, url)
        
        val imageRequestBuilder = ImageRequestBuilder.newBuilderWithSource(imageSource.uri)
        
        // Add TilePostprocessor when tiling is needed (bounds are guaranteed valid here)
        if (needsTiling()) {
            val postprocessor = MultiPostprocessor.from(listOf(tileProcessor))
            imageRequestBuilder.setPostprocessor(postprocessor)
        }
        
        val imageRequest: ImageRequest = ReactNetworkImageRequest.fromBuilderWithHeaders(
            imageRequestBuilder,
            null,
            imageSource.cacheControl
        )

        draweeHolder.controller = Fresco.newDraweeControllerBuilder()
            .setImageRequest(imageRequest)
            .setAutoPlayAnimations(true)
            .setOldController(draweeHolder.controller)
            .build()

        isDirty = false
    }
    private var view: View? = null

    fun onAttach(view: View) {
      this.view = view
      draweeHolder.onAttach()
    }

    fun onDetach(view: View) {
      draweeHolder.onDetach()
      if (this.view == view) {
        this.view = null
      }
    }

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

        val b = draweeHolder.isAttached
        val c = draweeHolder.isControllerValid
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
        val oldWidth = bounds.width()
        val oldHeight = bounds.height()
        val changed = bounds.left != left || bounds.top != top || 
                      bounds.right != right || bounds.bottom != bottom
        super.setBounds(left, top, right, bottom)
        draweeHolder.topLevelDrawable?.setBounds(left, top, right, bottom)
        
        if (changed) {
            val width = right - left
            val height = bottom - top
            
            // Mark dirty when:
            // 1. We're transitioning from invalid to valid bounds (first time we can load with tiling)
            // 2. OR dimensions changed and we need tiling (need to re-tile with new dimensions)
            val hadValidBounds = oldWidth > 0 && oldHeight > 0
            val hasValidBounds = width > 0 && height > 0
            val becameValid = !hadValidBounds && hasValidBounds
            
            if (needsTiling() && (becameValid || (hasValidBounds && (width != oldWidth || height != oldHeight)))) {
                isDirty = true
            }
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

  override fun invalidateDrawable(who: Drawable) {
    val x = 3
    view?.invalidate()
  }

  override fun scheduleDrawable(
    who: Drawable,
    what: Runnable,
    `when`: Long
  ) {
    val x = 3
  }

  override fun unscheduleDrawable(
    who: Drawable,
    what: Runnable
  ) {
    val x = 3
  }

  internal inner class TilePostprocessor : BasePostprocessor() {
    override fun process(
      source: Bitmap,
      bitmapFactory: PlatformBitmapFactory,
    ): CloseableReference<Bitmap> {
      val containerWidth = bounds.width()
      val containerHeight = bounds.height()

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

        val c = output.clone()
        this@ImageMaskDrawable.invalidateSelf()
        return c
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


