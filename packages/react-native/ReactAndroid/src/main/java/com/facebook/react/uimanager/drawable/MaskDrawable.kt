/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.drawable

import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.drawable.Drawable

/**
 * Common interface for mask Drawables that can be used to mask view content.
 * Both DraweeMaskDrawable and GradientMaskDrawable implement this interface.
 */
public interface MaskDrawable {
    /**
     * Draws this Drawable with Porter-Duff compositing applied.
     * This is called from ReactViewGroup.draw() to apply the mask.
     */
    public fun drawWithMaskMode(canvas: Canvas, maskPaint: Paint)
    public fun onAttach()
}
