/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.drawable.Drawable
import android.view.View
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.uimanager.style.BackgroundImageLayer
import com.facebook.react.uimanager.style.BackgroundPosition
import com.facebook.react.uimanager.style.BackgroundRepeat
import com.facebook.react.uimanager.style.BackgroundSize

/**
 * A wrapper that holds a BackgroundImageDrawable configured as a mask.
 * This provides a unified interface for both gradient and image masks.
 *
 * The underlying drawable can be accessed via the [drawable] property when needed.
 */
internal class MaskDrawable private constructor(
    /**
     * The underlying BackgroundImageDrawable used for masking.
     */
    val drawable: BackgroundImageDrawable
) {
    companion object {
        /**
         * Creates a MaskDrawable from a BackgroundImageDrawable.
         */
        fun fromBackgroundImageDrawable(backgroundImageDrawable: BackgroundImageDrawable): MaskDrawable {
            return MaskDrawable(backgroundImageDrawable)
        }

        /**
         * Creates a MaskDrawable from a mask image array (ReadableArray).
         * Parses the array and creates the appropriate mask drawable.
         *
         * This handles the core mask creation logic. View-specific concerns like layer type
         * and bounds should be handled by the caller.
         *
         * @param maskImageArray The mask image array from React Native props
         * @param context The Android context
         * @param existingMask Optional existing MaskDrawable to reuse
         * @return A MaskDrawable if creation succeeds, null if the array is empty or parsing fails
         */
        fun fromMaskImageArray(
            maskImageArray: ReadableArray?,
            context: Context,
            existingMask: MaskDrawable? = null
        ): MaskDrawable? {
            if (maskImageArray == null || maskImageArray.size() == 0) {
                return null
            }

            // Use the first mask layer
            val maskImageMap = maskImageArray.getMap(0) ?: return null
            
            // Parse the layer (supports both gradients and images)
            val layer = BackgroundImageLayer.parse(maskImageMap, context) ?: return null
            
            // Get or create BackgroundImageDrawable
            val backgroundImageDrawable = existingMask?.drawable
                ?: BackgroundImageDrawable(context, null, null)
            
            // Set the layer as a single-item list
            backgroundImageDrawable.backgroundImageLayers = listOf(layer)
            
            return MaskDrawable(backgroundImageDrawable)
        }
    }

    /**
     * Draws this Drawable with Porter-Duff compositing applied.
     * Delegates to the underlying BackgroundImageDrawable's drawWithMaskMode implementation.
     */
    fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
        drawable.drawWithMaskMode(canvas, maskPaint)
    }

    /**
     * Attaches the mask drawable (calls onAttach lifecycle method).
     * Delegates to the underlying BackgroundImageDrawable's onAttach implementation.
     */
    fun onAttach(view: View) {
        drawable.onAttach(view)
    }

    /**
     * Detaches the mask drawable (calls onDetach lifecycle method).
     * Delegates to the underlying BackgroundImageDrawable's onDetach implementation.
     */
    fun onDetach(view: View) {
        drawable.onDetach(view)
    }

    /**
     * Sets the size property on the underlying drawable.
     */
    fun setSize(size: BackgroundSize?) {
        drawable.backgroundSize = size?.let { listOf(it) }
    }

    /**
     * Sets the position property on the underlying drawable.
     */
    fun setPosition(position: BackgroundPosition?) {
        drawable.backgroundPosition = position?.let { listOf(it) }
    }

    /**
     * Sets the repeat property on the underlying drawable.
     */
    fun setRepeat(repeat: BackgroundRepeat?) {
        drawable.backgroundRepeat = repeat?.let { listOf(it) }
    }

    /**
     * Gets the size property from the underlying drawable.
     */
    val size: BackgroundSize?
        get() = drawable.backgroundSize?.firstOrNull()

    /**
     * Gets the position property from the underlying drawable.
     */
    val position: BackgroundPosition?
        get() = drawable.backgroundPosition?.firstOrNull()

    /**
     * Gets the repeat property from the underlying drawable.
     */
    val repeat: BackgroundRepeat?
        get() = drawable.backgroundRepeat?.firstOrNull()
}
