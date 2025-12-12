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
 * A wrapper that holds either a GradientMaskDrawable or ImageMaskDrawable
 * and provides a unified interface for mask-specific operations.
 *
 * The underlying drawable can be accessed via the [drawable] property when needed.
 */
internal class MaskDrawable private constructor(
    /**
     * The underlying drawable (either GradientMaskDrawable or ImageMaskDrawable).
     */
    val drawable: Drawable
) {
    companion object {
        /**
         * Creates a MaskDrawable from a GradientMaskDrawable.
         */
        fun fromGradient(gradientDrawable: GradientMaskDrawable): MaskDrawable {
            return MaskDrawable(gradientDrawable)
        }

        /**
         * Creates a MaskDrawable from an ImageMaskDrawable.
         */
        fun fromImage(imageDrawable: ImageMaskDrawable): MaskDrawable {
            return MaskDrawable(imageDrawable)
        }

        /**
         * Creates a gradient mask from a gradient definition.
         * Uses a GradientMaskDrawable to draw the gradient directly without creating a Bitmap copy.
         *
         * @param gradientMap The gradient definition map
         * @param context The Android context
         * @return A MaskDrawable wrapping a GradientMaskDrawable, or null if parsing fails
         */
        fun createGradientMask(gradientMap: ReadableMap, context: Context): MaskDrawable? {
            val gradientLayer = BackgroundImageLayer.parse(gradientMap, context)
            return gradientLayer?.let {
                MaskDrawable(GradientMaskDrawable(gradientLayer = it))
            }
        }

        /**
         * Loads an image mask using DraweeHolder.
         * This avoids creating Bitmap copies and leverages Fresco's caching and lifecycle management.
         *
         * The image will be loaded when the drawable has valid bounds (for tiling modes)
         * or immediately (for no-repeat mode).
         *
         * @param url The image URL to load
         * @param context The Android context
         * @param existingMask Optional existing MaskDrawable to reuse the ImageMaskDrawable from
         * @return A MaskDrawable wrapping an ImageMaskDrawable with the image URL set
         */
        fun loadImageMask(
            url: String,
            context: Context,
            existingMask: MaskDrawable? = null
        ): MaskDrawable {
            // Get or create ImageMaskDrawable
            val imageMaskDrawable = existingMask?.imageDrawable
                ?: ImageMaskDrawable(context)

            // Set the URL - ImageMaskDrawable handles request building internally
            // and will load when bounds are available (for tiling) or immediately (for no-repeat)
            imageMaskDrawable.setImageUrl(url)

            return MaskDrawable(imageMaskDrawable)
        }

        /**
         * Creates a MaskDrawable from a mask image array (ReadableArray).
         * Parses the array and creates the appropriate mask type (image or gradient).
         *
         * This handles the core mask creation logic. View-specific concerns like layer type
         * and bounds should be handled by the caller.
         *
         * @param maskImageArray The mask image array from React Native props
         * @param context The Android context
         * @param existingMask Optional existing MaskDrawable to reuse (for image masks)
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
            val type = maskImageMap.getString("type") ?: return null

            return when (type) {
                "image" -> {
                    val url = maskImageMap.getString("url")
                    if (url != null) {
                        loadImageMask(url, context, existingMask)
                    } else {
                        null
                    }
                }
                "linear-gradient", "radial-gradient" -> {
                    createGradientMask(maskImageMap, context)
                }
                else -> null
            }
        }
    }

    /**
     * Returns the underlying GradientMaskDrawable if this is a gradient type, null otherwise.
     */
    val gradientDrawable: GradientMaskDrawable?
        get() = drawable as? GradientMaskDrawable

    /**
     * Returns the underlying ImageMaskDrawable if this is an image type, null otherwise.
     */
    val imageDrawable: ImageMaskDrawable?
        get() = drawable as? ImageMaskDrawable

    /**
     * Returns true if this is a gradient mask, false if it's an image mask.
     */
    val isGradient: Boolean
        get() = drawable is GradientMaskDrawable

    /**
     * Returns true if this is an image mask, false if it's a gradient mask.
     */
    val isImage: Boolean
        get() = drawable is ImageMaskDrawable

    /**
     * Draws this Drawable with Porter-Duff compositing applied.
     * Delegates to the underlying drawable's drawWithMaskMode implementation.
     */
    fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.drawWithMaskMode(canvas, maskPaint)
            is ImageMaskDrawable -> d.drawWithMaskMode(canvas, maskPaint)
            else -> {
                // Fallback: delegate to standard draw if drawWithMaskMode is not available
                drawable.draw(canvas)
            }
        }
    }

    /**
     * Attaches the mask drawable (calls onAttach lifecycle method).
     * Delegates to the underlying drawable's onAttach implementation.
     */
    fun onAttach(view: View) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.onAttach()
            is ImageMaskDrawable -> d.onAttach(view)
        }
    }

    /**
     * Detaches the mask drawable (calls onDetach lifecycle method).
     * Delegates to the underlying drawable's onDetach implementation.
     */
    fun onDetach(view: View) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.onDetach()
            is ImageMaskDrawable -> d.onDetach(view)
        }
    }

    /**
     * Sets the size property on the underlying drawable.
     */
    fun setSize(size: BackgroundSize?) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.size = size
            is ImageMaskDrawable -> d.size = size
        }
    }

    /**
     * Sets the position property on the underlying drawable.
     */
    fun setPosition(position: BackgroundPosition?) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.position = position
            is ImageMaskDrawable -> d.position = position
        }
    }

    /**
     * Sets the repeat property on the underlying drawable.
     */
    fun setRepeat(repeat: BackgroundRepeat?) {
        when (val d = drawable) {
            is GradientMaskDrawable -> d.repeat = repeat
            is ImageMaskDrawable -> d.repeat = repeat
        }
    }

    /**
     * Gets the size property from the underlying drawable.
     */
    val size: BackgroundSize?
        get() = when (val d = drawable) {
            is GradientMaskDrawable -> d.size
            is ImageMaskDrawable -> d.size
            else -> null
        }

    /**
     * Gets the position property from the underlying drawable.
     */
    val position: BackgroundPosition?
        get() = when (val d = drawable) {
            is GradientMaskDrawable -> d.position
            is ImageMaskDrawable -> d.position
            else -> null
        }

    /**
     * Gets the repeat property from the underlying drawable.
     */
    val repeat: BackgroundRepeat?
        get() = when (val d = drawable) {
            is GradientMaskDrawable -> d.repeat
            is ImageMaskDrawable -> d.repeat
            else -> null
        }
}
