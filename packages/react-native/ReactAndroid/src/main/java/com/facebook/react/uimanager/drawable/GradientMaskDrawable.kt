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
import android.graphics.PorterDuff
import android.graphics.PorterDuffXfermode
import android.graphics.Shader
import android.graphics.drawable.Drawable

/**
 * A Drawable that draws a gradient shader for masking.
 * This avoids creating Bitmap copies - the gradient is drawn directly using a Shader.
 * 
 * The mask is applied using Porter-Duff DST_IN mode - the Drawable's alpha channel
 * determines what parts of the view are visible.
 */
internal class GradientMaskDrawable(
    private var shader: Shader? = null,
) : Drawable(), MaskDrawable {

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        shader = this@GradientMaskDrawable.shader
    }

    /**
     * Sets the Shader to use for the gradient mask.
     * The Shader will be drawn directly - no Bitmap conversion needed.
     */
    fun setShader(shader: Shader?) {
        if (this.shader != shader) {
            this.shader = shader
            paint.shader = shader
            invalidateSelf()
        }
    }

    override fun draw(canvas: Canvas) {
        val shader = this.shader ?: return
        val bounds = bounds
        
        if (bounds.isEmpty) {
            return
        }
        
        // Draw the gradient shader directly
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
        val shader = this.shader ?: return
        val bounds = bounds
        
        if (bounds.isEmpty) {
            return
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

    override fun onAttach() {
    }

    override fun setAlpha(alpha: Int) {
        paint.alpha = alpha
        invalidateSelf()
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        paint.colorFilter = colorFilter
        invalidateSelf()
    }

    @Deprecated("Deprecated in Java")
    override fun getOpacity(): Int {
        return PixelFormat.TRANSLUCENT
    }
}

