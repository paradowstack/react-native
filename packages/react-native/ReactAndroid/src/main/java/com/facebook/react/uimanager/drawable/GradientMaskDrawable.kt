/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.graphics.Canvas
import android.graphics.ColorFilter
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.Shader
import android.graphics.drawable.Drawable
import com.facebook.react.uimanager.style.BackgroundImageLayer

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
) : Drawable(), MaskDrawable {

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
    override fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
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
        val width = right - left
        val height = bottom - top

        gradientLayer?.takeIf { width > 0 && height > 0 }?.let { layer ->
            shader = layer.getShader(width.toFloat(), height.toFloat())
            super.setBounds(0, 0, width, height)
        }
    }

    override fun onAttach() = Unit

    override fun onDetach() = Unit

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
}

