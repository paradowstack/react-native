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
    System.out.println("Buffer length: " + length);
    
    emitOnMyBuffer(buffer);
  }

  @Override
  public void processBase64(String buffer) {
    int length = getBase64Length(buffer);
    System.out.println("Base64 buffer length: " + length);
    
    emitOnMyString("strong");
  }

  @Override
  public String processUnion(ReadableMap object) {
    if (object.hasKey("buffer") && !object.isNull("buffer")) {
      // Handle buffer case - buffers are passed as java.nio.ByteBuffer objects
      // The actual extraction would depend on how ByteBuffers are handled in unions
      System.out.println("Union buffer length: [ByteBuffer union processing]");
      return "buffer";
    } else if (object.hasKey("text") && !object.isNull("text")) {
      String textData = object.getString("text");
      int length = getBase64Length(textData);
      System.out.println("Union text length: " + length);
      return "text";
    } else {
      return "other";
    }
  }

  @Nullable
  @Override
  public ByteBuffer processArrayBufferStruct(ReadableMap object) {
    if (object.hasKey("buffer") && !object.isNull("buffer")) {
      // Handle buffer from struct - buffers are passed as java.nio.ByteBuffer objects
      // In React Native, ByteBuffers in structs need special handling
      // The actual extraction would depend on the bridge implementation
      
      // This is a placeholder for ByteBuffer extraction from ReadableMap
      System.out.println("Struct buffer length: [ByteBuffer struct processing]");
      
      // Note: The actual implementation would need to properly extract
      // the ByteBuffer from the ReadableMap structure
    }
    return null;
  }

  @Nullable
  @Override
  public String processStringStruct(ReadableMap object) {
    if (object.hasKey("text") && !object.isNull("text")) {
      String textData = object.getString("text");
      int length = getBase64Length(textData);
      System.out.println("Struct text length: " + length);
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
      // Handle first buffer in array - buffers are passed as java.nio.ByteBuffer objects
      // In React Native, ByteBuffers in arrays are typically accessed via native methods
      // For now, we'll simulate getting the first buffer
      // Note: Actual implementation may need to handle the ReadableArray differently
      // depending on how ByteBuffers are serialized in the bridge
      
      // This is a placeholder - the actual ByteBuffer extraction would depend on
      // the specific React Native bridge implementation for ByteBuffer arrays
      System.out.println("Buffer from Array length: [ByteBuffer array processing]");
      
      // Return a sample ByteBuffer for demonstration
      String sampleData = "sample-buffer-data";
      ByteBuffer buffer = ByteBuffer.wrap(sampleData.getBytes());
      System.out.println("Buffer from Array length: " + buffer.remaining());
      return buffer;
    }
    return null;
  }
}
