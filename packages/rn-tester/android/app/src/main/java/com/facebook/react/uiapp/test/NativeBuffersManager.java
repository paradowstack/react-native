package com.facebook.react.uiapp.test;

import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.Map;
import java.util.HashMap;
import java.util.Random;
import android.util.Base64;
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

  private Map<String, Object> constants;
  private ByteBuffer buffer;

  public NativeBuffersManager(ReactApplicationContext reactContext) {
    super(reactContext);

    constants = new HashMap<>();
    constants.put("text", "heavy-data");

    String heavyDataString = "heavy-data";
    byte[] randomBytes = new byte[4];
    new Random().nextBytes(randomBytes);
    buffer = ByteBuffer.wrap(randomBytes);
    constants.put("buffer", buffer);
  }

  private int getBase64Length(String base64String) {
    try {
      byte[] decodedBytes = Base64.decode(base64String, Base64.DEFAULT);
      return decodedBytes.length;
    } catch (Exception e) {
      return 0;
    }
  }

  @Override
  public void processBuffer(java.nio.ByteBuffer buffer) {
    int length = buffer.remaining();
    System.out.println("[JAVA] Buffer length: " + length);
    
    emitOnMyBuffer(buffer);
  }


  @Override
  public java.nio.ByteBuffer getBuffer() {
      byte[] array = new byte[4];
      new Random().nextBytes(array);
      return java.nio.ByteBuffer.wrap(array);
  }

  @Override
  public void getAsyncBuffer(Promise promise) {
    new Handler(Looper.getMainLooper()).postDelayed(() -> {
      byte[] array = new byte[4];
      new Random().nextBytes(array);
      promise.resolve(java.nio.ByteBuffer.wrap(array));
    }, 100);
  }

  @Override
  public void processBase64(String buffer) {
    int length = getBase64Length(buffer);
    System.out.println("[JAVA] Base64 buffer length: " + length);
    
    emitOnMyString("strong");
  }

  @Override
  public String processUnion(ReadableMap object) {
    if (object.hasKey("buffer") && !object.isNull("buffer")) {
      System.out.println("[JAVA] Union buffer type: " + object.getType("buffer").name());
      System.out.println("[JAVA] Union buffer length: " + object.getByteBuffer("buffer").remaining());
      return "buffer";
    } else if (object.hasKey("text") && !object.isNull("text")) {
      String textData = object.getString("text");
      int length = getBase64Length(textData);
      System.out.println("[JAVA] Union text length: " + length);
      return "text";
    } else {
      return "other";
    }
  }

  @Nullable
  @Override
  public ByteBuffer processArrayBufferStruct(ReadableMap object) {
    if (object.hasKey("buffer") && !object.isNull("buffer")) {
      System.out.println("[JAVA] Struct buffer length: " + object.getByteBuffer("buffer").remaining());
    }
    return null;
  }

  @Nullable
  @Override
  public String processStringStruct(ReadableMap object) {
    if (object.hasKey("text") && !object.isNull("text")) {
      String textData = object.getString("text");
      int length = getBase64Length(textData);
      System.out.println("[JAVA] Struct text length: " + length);
    }
    return null;
  }

  @Override
  protected Map<String, Object> getTypedExportedConstants() {
    return Map.of(
      "text", constants.get("text"),
      "buffer", buffer
    );
  }

  @Nullable
  @Override
  public ByteBuffer processArrayOfBuffers(ReadableArray buffers) {
    if (buffers.size() > 0) {
      System.out.println("[JAVA] Buffer from Array length: " + buffers.getByteBuffer(0).remaining());
      String sampleData = "sample-buffer-data";
      ByteBuffer buffer = ByteBuffer.wrap(sampleData.getBytes());
      System.out.println("[JAVA] Buffer from Array length: " + buffer.remaining());
      return buffer;
    }
    return null;
  }

  @Override
  public void processUnsafe(ReadableMap buffer) {
    
  }

  @Override
  public void processOptionalBuffer(@Nullable java.nio.ByteBuffer buffer) {
    if (buffer != null) {
      System.out.println("[JAVA] Optional buffer length: " + buffer.remaining());
    } else {
      System.out.println("[JAVA] Optional buffer is null");
    }
  }

}
