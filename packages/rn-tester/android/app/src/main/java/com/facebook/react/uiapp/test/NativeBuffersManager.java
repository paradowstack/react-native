package com.facebook.react.uiapp.test;

import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.Map;
import java.util.Random;
import com.facebook.fbreact.specs.NativeBuffersManagerSpec;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.Nullable;

import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.Callback;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.bridge.ReadableMap;
import com.facebook.react.bridge.Arguments;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.modules.core.DeviceEventManagerModule;

@ReactModule(name = NativeBuffersManager.NAME)
public class NativeBuffersManager extends NativeBuffersManagerSpec {

  public NativeBuffersManager(ReactApplicationContext reactContext) {
    super(reactContext);
  }
  @Override
  public void processBuffer(java.nio.ByteBuffer buffer) {
    System.out.println("Buffer length: " 
     + buffer.remaining());
  }

  @Override
  public void processBase64(String buffer) {
    System.out.println("Base64 length: " 
     + buffer.length());
  }

  @Override
  public String processUnion(ReadableMap object) {
    return "";
  }

  @Nullable
  @Override
  public ByteBuffer processArrayBufferStruct(ReadableMap object) {
    return null;
  }

  @Nullable
  @Override
  public String processStringStruct(ReadableMap object) {
    return "";
  }

  @Override
  protected Map<String, Object> getTypedExportedConstants() {
    return Collections.emptyMap();
  }

  @Nullable
  @Override
  public ByteBuffer processArrayOfBuffers(ReadableArray buffers) {
    return null;
  }
}
