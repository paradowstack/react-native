/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.graphics.Canvas
import android.graphics.ColorFilter
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.Shader
import android.graphics.drawable.Drawable
import com.facebook.react.uimanager.LengthPercentageType
import com.facebook.react.uimanager.PixelUtil.dpToPx
import com.facebook.react.uimanager.style.BackgroundImageLayer
import com.facebook.react.uimanager.style.BackgroundPosition
import com.facebook.react.uimanager.style.BackgroundRepeat
import com.facebook.react.uimanager.style.BackgroundSize

/**
 * A Drawable that draws a gradient shader for masking.
 * This avoids creating Bitmap copies - the gradient is drawn directly using a Shader.
 *
 * The mask is applied using Porter-Duff DST_IN mode - the Drawable's alpha channel
 * determines what parts of the view are visible.
 */
internal class GradientMaskDrawable(
    shader: Shader? = null,
    gradientLayer: BackgroundImageLayer? = null,
) : Drawable() {

    private var shader: Shader? = shader
        set(value) {
            if (field != value) {
                field = value
                paint.shader = value
                invalidateSelf()
            }
        }

    private var gradientLayer: BackgroundImageLayer? = gradientLayer
        set(value) {
            if (field != value) {
                field = value
                invalidateSelf()
            }
        }

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        this@GradientMaskDrawable.shader?.let { this.shader = it }
    }

    private val shaderMatrix = Matrix()

    var size: BackgroundSize? = null
        set(value) {
            if (field != value) {
                field = value
                updateShader()
                invalidateSelf()
            }
        }

    var position: BackgroundPosition? = null
        set(value) {
            if (field != value) {
                field = value
                updateShader()
                invalidateSelf()
            }
        }

    var repeat: BackgroundRepeat? = null
        set(value) {
            if (field != value) {
                field = value
                updateShader()
                invalidateSelf()
            }
        }

    private fun updateShader() {
        if (bounds.isEmpty) return
        val width = bounds.width().toFloat()
        val height = bounds.height().toFloat()
        
        gradientLayer?.takeIf { width > 0 && height > 0 }?.let { layer ->
            val baseShader = layer.getShader(width, height)
            
            // Apply size/position/repeat transforms via matrix
            shaderMatrix.reset()
            
            // Calculate effective size (gradients use container size by default)
            val (finalWidth, finalHeight) = calculateGradientSize(width, height)
            
            // Apply size scaling
            val scaleX = finalWidth / width
            val scaleY = finalHeight / height
            shaderMatrix.setScale(scaleX, scaleY)
            
            // Apply position offset (for no-repeat)
            val isNoRepeat = repeat?.x == com.facebook.react.uimanager.style.BackgroundRepeatKeyword.NoRepeat &&
                             repeat?.y == com.facebook.react.uimanager.style.BackgroundRepeatKeyword.NoRepeat
            if (isNoRepeat) {
                val (translateX, translateY) = calculateGradientPosition(width, height, finalWidth, finalHeight)
                shaderMatrix.postTranslate(translateX, translateY)
            }
            
            // Apply matrix to shader
            baseShader?.setLocalMatrix(shaderMatrix)
            shader = baseShader
        }
    }

    private fun calculateGradientSize(
        containerWidth: Float,
        containerHeight: Float
    ): Pair<Float, Float> {
        var finalWidth = containerWidth
        var finalHeight = containerHeight

        if (size is BackgroundSize.LengthPercentageAuto) {
            val lengthPercentage = (size as BackgroundSize.LengthPercentageAuto).lengthPercentage
            val w = lengthPercentage.x
            val h = lengthPercentage.y
            if (w != null && h != null) {
                finalWidth = positionToPixels(w, containerWidth)
                finalHeight = positionToPixels(h, containerHeight)
            } else if (w != null) {
                finalWidth = positionToPixels(w, containerWidth)
                finalHeight = containerHeight
            } else if (h != null) {
                finalHeight = positionToPixels(h, containerHeight)
                finalWidth = containerWidth
            }
        }

        return finalWidth to finalHeight
    }

    private fun calculateGradientPosition(
        containerWidth: Float,
        containerHeight: Float,
        gradientWidth: Float,
        gradientHeight: Float
    ): Pair<Float, Float> {
        val availableSpaceX = containerWidth - gradientWidth
        val availableSpaceY = containerHeight - gradientHeight

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

    override fun draw(canvas: Canvas) {
        if (paint.shader == null || bounds.isEmpty) return

        canvas.drawRect(
            bounds.left.toFloat(),
            bounds.top.toFloat(),
            bounds.right.toFloat(),
            bounds.bottom.toFloat(),
            paint
        )
    }

    /**
     * Draws this Drawable with Porter-Duff compositing applied.
     * This is called from ReactViewGroup.draw() to apply the mask.
     *
     * Since we're already inside a saveLayer in ReactViewGroup.draw(), we create
     * a nested saveLayer with Porter-Duff Paint. When we restore this inner layer,
     * it composites with the outer layer using Porter-Duff DST_IN mode.
     */
    fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
        if (paint.shader == null || bounds.isEmpty) return

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

        // Draw the gradient shader to the inner layer
        canvas.drawRect(
            bounds.left.toFloat(),
            bounds.top.toFloat(),
            bounds.right.toFloat(),
            bounds.bottom.toFloat(),
            paint
        )

        // Restore the inner layer - this composites it with the outer layer using DST_IN mode
        canvas.restoreToCount(saveCount)
    }

    override fun setBounds(left: Int, top: Int, right: Int, bottom: Int) {
        super.setBounds(left, top, right, bottom)
        updateShader()
    }

    fun onAttach() = Unit

    fun onDetach() = Unit

    override fun setAlpha(alpha: Int) {
        paint.alpha = alpha
        invalidateSelf()
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        paint.colorFilter = colorFilter
        invalidateSelf()
    }

    @Deprecated("Deprecated in Java")
    override fun getOpacity(): Int = PixelFormat.TRANSLUCENT

  override fun getIntrinsicWidth(): Int  = -1
  override fun getIntrinsicHeight(): Int  = -1
}

