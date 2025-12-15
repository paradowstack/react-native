/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.react.uimanager.style

import android.content.Context
import android.graphics.Shader
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.facebook.react.views.imagehelper.ImageSource

public class BackgroundImageLayer private constructor(
  private val gradient: Gradient?,
  private val imageSource: ImageSource?
) {
  
  public val isGradient: Boolean = gradient != null
  public val isImage: Boolean = imageSource != null

  public companion object {
    public fun parse(layerMap: ReadableMap?, context: Context): BackgroundImageLayer? {
      if (layerMap == null) {
        return null
      }
      
      if (!layerMap.hasKey("type") || layerMap.getType("type") != ReadableType.String) {
        return null
      }

      return when (layerMap.getString("type")) {
        "linear-gradient" -> {
          val gradient = LinearGradient.parse(layerMap, context)
          if (gradient != null) BackgroundImageLayer(gradient, null) else null
        }
        "radial-gradient" -> {
          val gradient = RadialGradient.parse(layerMap, context)
          if (gradient != null) BackgroundImageLayer(gradient, null) else null
        }
        "image" -> {
          val url = layerMap.getString("url")
          if (url != null) {
            val imageSource = ImageSource(context, url)
            BackgroundImageLayer(null, imageSource)
          } else {
            null
          }
        }
        else -> null
      }
    }
  }

  public fun getShader(width: Float, height: Float): Shader? = gradient?.getShader(width, height)
  
  public fun getImageSource(): ImageSource? = imageSource
}
