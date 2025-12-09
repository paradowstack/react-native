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
import android.graphics.drawable.Drawable
import com.facebook.drawee.generic.GenericDraweeHierarchy
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder
import com.facebook.drawee.view.DraweeHolder

/**
 * A Drawable that uses DraweeHolder to load and display an image mask.
 * This avoids creating Bitmap copies and leverages Fresco's caching and lifecycle management.
 *
 * The mask is applied using Porter-Duff DST_IN mode - the Drawable's alpha channel
 * determines what parts of the view are visible.
 */
internal class DraweeMaskDrawable(
    resources: android.content.res.Resources,
) : Drawable(), MaskDrawable {

    private val draweeHolder: DraweeHolder<GenericDraweeHierarchy> =
        DraweeHolder(GenericDraweeHierarchyBuilder.newInstance(resources).setFadeDuration(0).build())

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

    override fun onAttach() = draweeHolder.onAttach()

    override fun onDetach() = draweeHolder.onDetach()

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
    override fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
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
        super.setBounds(left, top, right, bottom)
        draweeHolder.topLevelDrawable?.setBounds(left, top, right, bottom)
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
}

