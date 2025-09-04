package com.facebook.react.uiapp.test;

import java.util.Random;
import com.facebook.fbreact.specs.NativeBuffersManagerSpec;

import android.os.Handler;
import android.os.Looper;

import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.Callback;
import com.facebook.react.bridge.ReactMethod;
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
}







