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
import android.graphics.drawable.Animatable
import android.graphics.drawable.Drawable
import com.facebook.drawee.controller.ControllerListener
import com.facebook.drawee.drawable.DrawableParent
import com.facebook.drawee.generic.GenericDraweeHierarchy
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder
import com.facebook.drawee.view.DraweeHolder
import com.facebook.imagepipeline.image.ImageInfo
import com.facebook.react.views.image.ImageResizeMode

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


    // Note: Porter-Duff compositing is applied at the view level, not here
    // This Drawable just draws the image normally - its alpha channel will be used for masking

    init {
        // Set ourselves as callback to invalidate when DraweeDrawable changes
        // This will be updated when the controller is set
    }
    
    /**
     * Sets the ImageRequest to load as the mask.
     * The DraweeHolder will handle loading, caching, and lifecycle management.
     */
    fun setImageRequest(imageRequest: com.facebook.imagepipeline.request.ImageRequest?) {
        if (imageRequest == null) {
            draweeHolder.controller = null
            return
        }

        val controllerBuilder = com.facebook.drawee.backends.pipeline.Fresco.newDraweeControllerBuilder()
        controllerBuilder
            .setImageRequest(imageRequest)
            .setAutoPlayAnimations(true)
            .setOldController(draweeHolder.controller)

        draweeHolder.controller = controllerBuilder.build()
        controllerBuilder.reset()
    }

    /**
     * Attaches the DraweeHolder (should be called when view is attached to window)
     */
    override fun onAttach() {
        draweeHolder.onAttach()
    }

    /**
     * Detaches the DraweeHolder (should be called when view is detached from window)
     */
    fun onDetach() {
        draweeHolder.onDetach()
    }

    override fun draw(canvas: Canvas) {
        val drawable = draweeHolder.topLevelDrawable ?: return
        val bounds = bounds
        
        // Set bounds if not already set
        if (drawable.bounds.isEmpty) {
          drawable.bounds = bounds
        }
        
        // Draw the DraweeDrawable normally
        // The Porter-Duff DST_IN mode will be applied at the view level when this Drawable
        // is drawn with a Paint that has the xfermode set
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
        val bounds = bounds
        
        if (bounds.isEmpty) {
            return
        }
        
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
    override fun getOpacity(): Int {
        return draweeHolder.topLevelDrawable?.opacity ?: PixelFormat.TRANSLUCENT
    }
}

