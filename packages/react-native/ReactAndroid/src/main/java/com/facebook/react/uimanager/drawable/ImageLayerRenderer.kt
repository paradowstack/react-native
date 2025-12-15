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
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.Shader.TileMode
import android.graphics.drawable.Drawable
import android.view.View
import com.facebook.common.references.CloseableReference
import com.facebook.drawee.backends.pipeline.Fresco
import com.facebook.drawee.drawable.ScalingUtils
import com.facebook.drawee.generic.GenericDraweeHierarchy
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder
import com.facebook.drawee.view.DraweeHolder
import com.facebook.imagepipeline.bitmaps.PlatformBitmapFactory
import com.facebook.imagepipeline.request.BasePostprocessor
import com.facebook.imagepipeline.request.ImageRequest
import com.facebook.imagepipeline.request.ImageRequestBuilder
import com.facebook.react.modules.fresco.ReactNetworkImageRequest
import com.facebook.react.uimanager.FloatUtil
import com.facebook.react.uimanager.LengthPercentageType
import com.facebook.react.uimanager.PixelUtil.dpToPx
import com.facebook.react.uimanager.style.BackgroundPosition
import com.facebook.react.uimanager.style.BackgroundRepeat
import com.facebook.react.uimanager.style.BackgroundRepeatKeyword
import com.facebook.react.uimanager.style.BackgroundSize
import com.facebook.react.views.image.MultiPostprocessor
import com.facebook.react.views.imagehelper.ImageSource
import kotlin.math.ceil
import kotlin.math.floor
import kotlin.math.round

/**
 * Helper class that renders image layers using Fresco's DraweeHolder.
 * Handles tiling, positioning, sizing, and repeat modes for background/mask images.
 *
 * This class manages a single image layer with its own DraweeHolder lifecycle.
 */
internal class ImageLayerRenderer(
    private val context: Context,
    private val imageSource: ImageSource
) : Drawable.Callback {
    
    private val tileProcessor: TilePostprocessor = TilePostprocessor()
    private val scaleType: PositioningScaleType = PositioningScaleType()
    
    private val draweeHolder: DraweeHolder<GenericDraweeHierarchy> =
        DraweeHolder(GenericDraweeHierarchyBuilder.newInstance(context.resources)
            .setFadeDuration(0)
            .setActualImageScaleType(scaleType)
            .build()).apply { this.topLevelDrawable?.callback = this@ImageLayerRenderer }
    
    private var view: View? = null
    private var isDirty: Boolean = true
    private var bounds: Rect = Rect()
    
    var size: BackgroundSize? = null
        set(value) {
            if (field != value) {
                field = value
                scaleType.invalidate()
                markDirty()
            }
        }
    
    var position: BackgroundPosition? = null
        set(value) {
            if (field != value) {
                field = value
                scaleType.invalidate()
                markDirty()
            }
        }
    
    var repeat: BackgroundRepeat? = null
        set(value) {
            if (field != value) {
                field = value
                markDirty()
            }
        }
    
    private var positioningArea: RectF = RectF()
    
    /**
     * Returns true if tiling is needed (any repeat mode other than no-repeat on both axes).
     */
    private fun needsTiling(): Boolean {
        val r = repeat ?: return true // Default is repeat
        return r.x != BackgroundRepeatKeyword.NoRepeat || r.y != BackgroundRepeatKeyword.NoRepeat
    }
    
    fun setBounds(bounds: Rect, positioningArea: RectF) {
        val source = imageSource.uri
        val changed = this.bounds != bounds || this.positioningArea != positioningArea
        if (changed) {
            val hadValidBounds = this.bounds.width() > 0 && this.bounds.height() > 0
            val hasValidBounds = bounds.width() > 0 && bounds.height() > 0
            val becameValid = !hadValidBounds && hasValidBounds

            this.bounds = Rect(bounds)
            this.positioningArea = RectF(positioningArea)
            draweeHolder.topLevelDrawable?.setBounds(bounds)

            if (becameValid) {
                markDirty()
            }

            scaleType.invalidate()
        }
    }
    
    private fun markDirty() {
        isDirty = true
        updateImageRequest()
    }
    
    private fun updateImageRequest() {
        if (!isDirty || bounds.width() <= 0 || bounds.height() <= 0) {
            return
        }
        
        val imageRequestBuilder = ImageRequestBuilder.newBuilderWithSource(imageSource.uri)
        
        // Add TilePostprocessor when tiling is needed
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
    
    fun onAttach(view: View) {
        this.view = view
        draweeHolder.onAttach()
        updateImageRequest()
    }
    
    fun onDetach(view: View) {
        draweeHolder.onDetach()
        if (this.view == view) {
            this.view = null
        }
    }
    
    fun draw(canvas: Canvas, paintingArea: RectF) {
        val drawable = draweeHolder.topLevelDrawable ?: return
        
        if (drawable.bounds.isEmpty) {
            drawable.bounds = bounds
        }
        
        // The PositioningScaleType already handles the positioning/sizing
        // Just draw the drawable - it will be clipped by the parent's clipPath
        drawable.draw(canvas)
    }
    
    override fun invalidateDrawable(who: Drawable) {
        view?.invalidate()
    }
    
    override fun scheduleDrawable(who: Drawable, what: Runnable, `when`: Long) {
    }
    
    override fun unscheduleDrawable(who: Drawable, what: Runnable) {
    }
    
    private fun positionToPixels(
        lengthPercentage: com.facebook.react.uimanager.LengthPercentage,
        availableSpace: Float
    ): Float =
        if (lengthPercentage.type == LengthPercentageType.PERCENT) {
            lengthPercentage.resolve(availableSpace)
        } else {
            lengthPercentage.resolve(availableSpace).dpToPx()
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
    
    internal inner class PositioningScaleType : ScalingUtils.ScaleType, ScalingUtils.StatefulScaleType {
        private var state = Any()
        
        fun invalidate() {
            state = Any()
        }
        
        override fun getTransform(
            outTransform: Matrix,
            parentBounds: Rect,
            childWidth: Int,
            childHeight: Int,
            focusX: Float,
            focusY: Float
        ): Matrix {
            val containerWidth = positioningArea.width()
            val containerHeight = positioningArea.height()
            
            outTransform.reset()
            if (containerWidth <= 0 || containerHeight <= 0 || childWidth <= 0 || childHeight <= 0 || needsTiling()) {
                return outTransform
            }
            
            val (finalWidth, finalHeight) = calculateImageSize(
                containerWidth,
                containerHeight,
                childWidth.toFloat(),
                childHeight.toFloat()
            )
            
            val (translateX, translateY) = calculatePosition(
                containerWidth,
                containerHeight,
                finalWidth,
                finalHeight
            )
            
            val scaleX = finalWidth / childWidth
            val scaleY = finalHeight / childHeight
            
            outTransform.setScale(scaleX, scaleY)
            outTransform.postTranslate(translateX + positioningArea.left, translateY + positioningArea.top)
            
            return outTransform
        }
        
        override fun getState(): Any = state
    }
    
    internal inner class TilePostprocessor : BasePostprocessor() {
        override fun process(
            sourceBitmap: Bitmap,
            bitmapFactory: PlatformBitmapFactory
        ): CloseableReference<Bitmap> {
            val containerWidth = bounds.width()
            val containerHeight = bounds.height()
            val imageWidth = sourceBitmap.width.toFloat()
            val imageHeight = sourceBitmap.height.toFloat()
            
            if (containerWidth <= 0 || containerHeight <= 0 || imageWidth <= 0 || imageHeight <= 0) {
                return super.process(sourceBitmap, bitmapFactory)
            }
            
            val (tileWidth, tileHeight) = calculateImageSize(
                positioningArea.width(),
                positioningArea.height(),
                imageWidth,
                imageHeight
            )
            
            if (tileWidth <= 0 || tileHeight <= 0) {
              return super.process(sourceBitmap, bitmapFactory)
            }
            
            val r = repeat ?: BackgroundRepeat(BackgroundRepeatKeyword.Repeat, BackgroundRepeatKeyword.Repeat)
            
            // Decide rendering strategy based on repeat mode
            val useShader = (r.x == BackgroundRepeatKeyword.Repeat && r.y == BackgroundRepeatKeyword.Repeat) ||
                            (r.x == BackgroundRepeatKeyword.Repeat && r.y == BackgroundRepeatKeyword.NoRepeat) ||
                            (r.x == BackgroundRepeatKeyword.NoRepeat && r.y == BackgroundRepeatKeyword.Repeat)
            
            return if (useShader) {
                drawWithShader(sourceBitmap, bitmapFactory, containerWidth, containerHeight, tileWidth, tileHeight, r)
            } else {
                drawTilesManually(sourceBitmap, bitmapFactory, containerWidth, containerHeight, tileWidth, tileHeight, r)
            }
        }
        
        private fun drawWithShader(
            sourceBitmap: Bitmap,
            bitmapFactory: PlatformBitmapFactory,
            containerWidth: Int,
            containerHeight: Int,
            tileWidth: Float,
            tileHeight: Float,
            repeat: BackgroundRepeat
        ): CloseableReference<Bitmap> {
            val tileBitmapRef = bitmapFactory.createBitmap(
                tileWidth.toInt().coerceAtLeast(1),
                tileHeight.toInt().coerceAtLeast(1)
            )
            val tileBitmap = tileBitmapRef.get()
            val tileCanvas = Canvas(tileBitmap)
            
            val paint = Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
            val scaleX = tileWidth / sourceBitmap.width
            val scaleY = tileHeight / sourceBitmap.height
            
            val matrix = Matrix()
            matrix.setScale(scaleX, scaleY)
            tileCanvas.drawBitmap(sourceBitmap, matrix, paint)
            
            val outputRef = bitmapFactory.createBitmap(containerWidth, containerHeight)
            val outputBitmap = outputRef.get()
            val outputCanvas = Canvas(outputBitmap)
            
            val tileModeX = if (repeat.x == BackgroundRepeatKeyword.Repeat) TileMode.REPEAT else TileMode.CLAMP
            val tileModeY = if (repeat.y == BackgroundRepeatKeyword.Repeat) TileMode.REPEAT else TileMode.CLAMP
            
            val shader = BitmapShader(tileBitmap, tileModeX, tileModeY)
            val shaderPaint = Paint(Paint.ANTI_ALIAS_FLAG)
            shaderPaint.shader = shader
            
            val (translateX, translateY) = calculatePosition(
                positioningArea.width(),
                positioningArea.height(),
                tileWidth,
                tileHeight
            )
            
            val shaderMatrix = Matrix()
            shaderMatrix.setTranslate(translateX, translateY)
            shader.setLocalMatrix(shaderMatrix)
            
            outputCanvas.drawRect(0f, 0f, containerWidth.toFloat(), containerHeight.toFloat(), shaderPaint)
            
            tileBitmapRef.close()
            return outputRef
        }
        
        private fun drawTilesManually(
            sourceBitmap: Bitmap,
            bitmapFactory: PlatformBitmapFactory,
            containerWidth: Int,
            containerHeight: Int,
            tileWidth: Float,
            tileHeight: Float,
            repeat: BackgroundRepeat
        ): CloseableReference<Bitmap> {
            val outputRef = bitmapFactory.createBitmap(containerWidth, containerHeight)
            val outputBitmap = outputRef.get()
            val canvas = Canvas(outputBitmap)
            
            val paint = Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
            
            var (initialX, initialY) = calculatePosition(
                positioningArea.width(),
                positioningArea.height(),
                tileWidth,
                tileHeight
            )
            
            val repeatX = repeat.x
            var xTilesCount = 1
            var xSpacing = 0f
            
            if (repeatX == BackgroundRepeatKeyword.Space) {
                val widthOfEdgePinnedImages = tileWidth * 2
                val availableWidthForCenterImages = positioningArea.width() - widthOfEdgePinnedImages
                val roundedTileWidth = round(tileWidth)
                if (roundedTileWidth > 0 && (availableWidthForCenterImages > 0 || FloatUtil.floatsEqual(availableWidthForCenterImages, 0f))) {
                    val centerImagesCount = floor(round(availableWidthForCenterImages) / roundedTileWidth).toInt()
                    val centerImagesWidth = centerImagesCount * tileWidth
                    val totalFreeSpace = availableWidthForCenterImages - centerImagesWidth
                    val totalInstances = centerImagesCount + 2
                    xSpacing = totalFreeSpace / (totalInstances - 1)
                    xTilesCount = totalInstances
                    initialX = positioningArea.left
                } else {
                    xTilesCount = 1
                }
            } else if (repeatX == BackgroundRepeatKeyword.Round || repeatX == BackgroundRepeatKeyword.Repeat) {
                val roundedTileWidth = round(tileWidth)
                if (roundedTileWidth > 0) {
                    val tilesBeforeX = ceil(round(initialX + positioningArea.left) / roundedTileWidth).toInt()
                    val tilesAfterX = ceil(round((positioningArea.width() - initialX)) / roundedTileWidth).toInt()
                    xTilesCount = tilesBeforeX + tilesAfterX
                    initialX = positioningArea.left - (tilesBeforeX * tileWidth)
                }
                xSpacing = 0f
            } else {
                initialX += positioningArea.left
            }
            
            val repeatY = repeat.y
            var yTilesCount = 1
            var ySpacing = 0f
            
            if (repeatY == BackgroundRepeatKeyword.Space) {
                val heightOfEdgePinnedImages = tileHeight * 2
                val availableHeightForCenterImages = positioningArea.height() - heightOfEdgePinnedImages
                val roundedTileHeight = round(tileHeight)
                if (roundedTileHeight > 0 && (availableHeightForCenterImages > 0 || FloatUtil.floatsEqual(availableHeightForCenterImages, 0f))) {
                    val centerImagesCount = floor(round(availableHeightForCenterImages) / roundedTileHeight).toInt()
                    val centerImagesHeight = centerImagesCount * tileHeight
                    val totalFreeSpace = availableHeightForCenterImages - centerImagesHeight
                    val totalInstances = centerImagesCount + 2
                    ySpacing = totalFreeSpace / (totalInstances - 1)
                    yTilesCount = totalInstances
                    initialY = positioningArea.top
                } else {
                    yTilesCount = 1
                }
            } else if (repeatY == BackgroundRepeatKeyword.Round || repeatY == BackgroundRepeatKeyword.Repeat) {
                val roundedTileHeight = round(tileHeight)
                if (roundedTileHeight > 0) {
                    val tilesBeforeY = ceil(round(initialY + positioningArea.top) / roundedTileHeight).toInt()
                    val tilesAfterY = ceil(round((positioningArea.height() - initialY)) / roundedTileHeight).toInt()
                    yTilesCount = tilesBeforeY + tilesAfterY
                    initialY = positioningArea.top - (tilesBeforeY * tileHeight)
                }
                ySpacing = 0f
            } else {
                initialY += positioningArea.top
            }
            
            val scaleX = tileWidth / sourceBitmap.width
            val scaleY = tileHeight / sourceBitmap.height
            
            var translateX = initialX
            var translateY: Float
            repeat(xTilesCount) {
                translateY = initialY
                repeat(yTilesCount) {
                    canvas.save()
                    canvas.translate(translateX, translateY)
                    canvas.scale(scaleX, scaleY)
                    canvas.drawBitmap(sourceBitmap, 0f, 0f, paint)
                    canvas.restore()
                    translateY += tileHeight + ySpacing
                }
                translateX += tileWidth + xSpacing
            }
            
            return outputRef
        }
    }
}
