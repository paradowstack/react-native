/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#if __has_include(<ReactCodegen/AppSpecsJSI.h>) // CocoaPod headers on Apple
#include <ReactCodegen/AppSpecsJSI.h>
#elif __has_include("AppSpecsJSI.h") // Cmake headers on Android
#include "AppSpecsJSI.h"
#else // BUCK headers
#include <AppSpecs/AppSpecsJSI.h>
#endif
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace facebook::react {

#pragma mark - Structs - Structs
using ConstantsStruct =
    NativeCxxModuleExampleConstantsStruct<bool, int32_t, std::string>;

template <>
struct Bridging<ConstantsStruct>
    : NativeCxxModuleExampleConstantsStructBridging<ConstantsStruct> {};

using ObjectStruct = NativeCxxModuleExampleObjectStruct<
    int32_t,
    std::string,
    std::optional<std::string>>;

template <>
struct Bridging<ObjectStruct>
    : NativeCxxModuleExampleObjectStructBridging<ObjectStruct> {};

using ValueStruct = NativeCxxModuleExampleValueStruct<
    double,
    std::string,
    ObjectStruct,
    jsi::ArrayBuffer>;

template <>
struct Bridging<ValueStruct>
    : NativeCxxModuleExampleValueStructBridging<ValueStruct> {};

using BufferStruct =
    NativeCxxModuleExampleBufferStruct<std::string, jsi::ArrayBuffer>;

template <>
struct Bridging<BufferStruct>
    : NativeCxxModuleExampleBufferStructBridging<BufferStruct> {};

#pragma mark - enums
enum class CustomEnumInt : int32_t { A = 23, B = 42 };

template <>
struct Bridging<CustomEnumInt> {
  static CustomEnumInt fromJs(jsi::Runtime& rt, jsi::Value rawValue) {
    auto value = static_cast<int32_t>(rawValue.asNumber());
    if (value == 23) {
      return CustomEnumInt::A;
    } else if (value == 42) {
      return CustomEnumInt::B;
    } else {
      throw jsi::JSError(rt, "Invalid enum value");
    }
  }

  static int32_t toJs(jsi::Runtime& rt, CustomEnumInt value) {
    return bridging::toJs(rt, static_cast<int32_t>(value));
  }
};

#pragma mark - jsi::HostObjects
template <typename T>
class HostObjectWrapper : public jsi::HostObject {
 public:
  HostObjectWrapper(std::shared_ptr<T> value) : value_(std::move(value)) {}

  std::shared_ptr<T> getValue() const {
    return value_;
  }

  ~HostObjectWrapper() override = default;

 private:
  std::shared_ptr<T> value_;
};

struct CustomHostObjectRef {
  CustomHostObjectRef(std::string a, int32_t b) : a_(a), b_(b) {}
  std::string a_;
  int32_t b_;
};

using CustomHostObject = HostObjectWrapper<CustomHostObjectRef>;

#pragma mark - recursive objects

using BinaryTreeNode = NativeCxxModuleExampleBinaryTreeNode<int32_t>;

template <>
struct Bridging<BinaryTreeNode>
    : NativeCxxModuleExampleBinaryTreeNodeBridging<BinaryTreeNode> {};

using GraphNode = NativeCxxModuleExampleGraphNode<std::string>;

template <>
struct Bridging<GraphNode>
    : NativeCxxModuleExampleGraphNodeBridging<GraphNode> {};

#pragma mark - functional object properties

using MenuItem = NativeCxxModuleExampleMenuItem<
    std::string,
    AsyncCallback<std::string, bool>,
    std::optional<std::string>>;

template <>
struct Bridging<MenuItem> : NativeCxxModuleExampleMenuItemBridging<MenuItem> {};

#pragma mark - RCTDeviceEventEmitter events

using CustomDeviceEvent = NativeCxxModuleExampleCustomDeviceEvent<
    std::string,
    int32_t,
    std::optional<float>>;

template <>
struct Bridging<CustomDeviceEvent>
    : NativeCxxModuleExampleCustomDeviceEventBridging<CustomDeviceEvent> {};

class NSDataMutableBuffer : public facebook::jsi::MutableBuffer {
 public:
  NSDataMutableBuffer(uint8_t* data, size_t size) : _data(data), _size(size) {}
  NSDataMutableBuffer(const NSDataMutableBuffer& other)
      : _data(other._data), _size(other._size) {}

  NSDataMutableBuffer(NSDataMutableBuffer&& other) noexcept
      : _data(other._data), _size(other._size) {
    other._data = nullptr;
    other._size = 0;
  }

  NSDataMutableBuffer& operator=(const NSDataMutableBuffer& other) {
    if (this != &other) {
      _data = other._data;
      _size = other._size;
    }
    return *this;
  }

  NSDataMutableBuffer& operator=(NSDataMutableBuffer&& other) noexcept {
    if (this != &other) {
      _data = other._data;
      _size = other._size;
      other._data = nullptr;
      other._size = 0;
    }
    return *this;
  }
  uint8_t* data() override {
    return _data;
  }
  size_t size() const override {
    return _size;
  }

 private:
  uint8_t* _data;
  size_t _size;
};

#pragma mark - implementation
class NativeCxxModuleExample
    : public NativeCxxModuleExampleCxxSpec<NativeCxxModuleExample> {
 public:
  NativeCxxModuleExample(std::shared_ptr<CallInvoker> jsInvoker);

  void processBufferUnion(jsi::Runtime& rt, jsi::Object arg) {
    if (arg.hasProperty(rt, "value")) {
      auto value = arg.getProperty(rt, "value").asNumber();
      std::cout << "Received union with value: " << value << std::endl;
    } else if (arg.hasProperty(rt, "buffer")) {
      auto buffer =
          arg.getProperty(rt, "buffer").asObject(rt).asArrayBuffer(rt);
      std::cout << "Received union with buffer size: " << buffer.size(rt)
                << std::endl;

    } else {
      throw jsi::JSError(rt, "Invalid union object");
    }
  }
  BufferStruct getBufferStruct(jsi::Runtime& rt) {
    return {"text", getBuffer(rt)};
  }
  void processBufferStruct(jsi::Runtime& rt, BufferStruct arg) {
    std::cout << "Received struct with text: " << arg.text
              << " and buffer size: " << arg.value.size(rt) << std::endl;
  }

  std::string printBuffer(jsi::Runtime& rt, jsi::ArrayBuffer& buffer) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < buffer.size(rt); ++i) {
      oss << static_cast<int>(buffer.data(rt)[i]);
      if (i < buffer.size(rt) - 1) {
        oss << ", ";
      }
    }
    oss << "]";
    return oss.str();
  }

  jsi::ArrayBuffer getBuffer(jsi::Runtime& rt) {
    auto length = 8;
    auto data = new uint8_t[length];
    for (size_t i = 0; i < length; ++i) {
      data[i] = static_cast<uint8_t>(rand() % 256);
    }
    auto mutableBuffer = std::make_shared<NSDataMutableBuffer>(data, length);
    auto arrayBuffer = jsi::ArrayBuffer(rt, mutableBuffer);
    return arrayBuffer;
  }

  AsyncPromise<jsi::ArrayBuffer> returningBuffer(jsi::Runtime& rt) {
    auto promise = AsyncPromise<jsi::ArrayBuffer>(rt, jsInvoker_);
    auto length = 8;
    auto data = new uint8_t[length];
    for (size_t i = 0; i < length; ++i) {
      data[i] = static_cast<uint8_t>(rand() % 256);
    }
    auto mutableBuffer = std::make_shared<NSDataMutableBuffer>(data, length);
    auto arrayBuffer = jsi::ArrayBuffer(rt, mutableBuffer);

    std::cout << "Sending buffer: " << printBuffer(rt, arrayBuffer)
              << std::endl;
    promise.resolve(std::move(arrayBuffer));
    return promise;
  }
  void takingBuffer(jsi::Runtime& rt, jsi::ArrayBuffer buffer) {
    std::cout << "Received buffer: " << printBuffer(rt, buffer);
    std::cout << "]" << std::endl;
  }

  void getValueWithCallback(
      jsi::Runtime& rt,
      const AsyncCallback<std::string>& callback);

  std::function<void()> setValueCallbackWithSubscription(
      jsi::Runtime& rt,
      AsyncCallback<std::string> callback);

  std::vector<std::optional<ObjectStruct>> getArray(
      jsi::Runtime& rt,
      std::vector<std::optional<ObjectStruct>> arg);

  bool getBool(jsi::Runtime& rt, bool arg);

  ConstantsStruct getConstants(jsi::Runtime& rt);

  CustomEnumInt getCustomEnum(jsi::Runtime& rt, CustomEnumInt arg);

  std::shared_ptr<CustomHostObject> getCustomHostObject(jsi::Runtime& rt);

  std::string consumeCustomHostObject(
      jsi::Runtime& rt,
      std::shared_ptr<CustomHostObject> arg);
  std::string vomitCustomHostObject(
      jsi::Runtime& rt,
      std::shared_ptr<CustomHostObject> arg);

  BinaryTreeNode getBinaryTreeNode(jsi::Runtime& rt, BinaryTreeNode arg);

  GraphNode getGraphNode(jsi::Runtime& rt, GraphNode arg);

  NativeCxxModuleExampleEnumInt getNumEnum(
      jsi::Runtime& rt,
      NativeCxxModuleExampleEnumInt arg);

  NativeCxxModuleExampleEnumStr getStrEnum(
      jsi::Runtime& rt,
      NativeCxxModuleExampleEnumNone arg);

  std::map<std::string, std::optional<int32_t>> getMap(
      jsi::Runtime& rt,
      std::map<std::string, std::optional<int32_t>> arg);

  double getNumber(jsi::Runtime& rt, double arg);

  ObjectStruct getObject(jsi::Runtime& rt, ObjectStruct arg);

  std::set<float> getSet(jsi::Runtime& rt, std::set<float> arg);

  std::string getString(jsi::Runtime& rt, std::string arg);

  std::string
  getUnion(jsi::Runtime& rt, float x, const std::string& y, jsi::Object z);

  ValueStruct getValue(
      jsi::Runtime& rt,
      double x,
      std::string y,
      ObjectStruct z,
      jsi::ArrayBuffer a);

  AsyncPromise<std::string> getValueWithPromise(jsi::Runtime& rt, bool error);

  std::optional<bool> getWithWithOptionalArgs(
      jsi::Runtime& rt,
      std::optional<bool> optionalArg);

  void voidFunc(jsi::Runtime& rt);

  AsyncPromise<> voidPromise(jsi::Runtime& rt);

  void setMenu(jsi::Runtime& rt, MenuItem menuItem);

  void emitCustomDeviceEvent(jsi::Runtime& rt, const std::string& eventName);

  void voidFuncThrows(jsi::Runtime& rt);

  ObjectStruct getObjectThrows(jsi::Runtime& rt, const ObjectStruct& arg);

  AsyncPromise<> promiseThrows(jsi::Runtime& rt);

  void voidFuncAssert(jsi::Runtime& rt);

  ObjectStruct getObjectAssert(jsi::Runtime& rt, const ObjectStruct& arg);

  AsyncPromise<> promiseAssert(jsi::Runtime& rt);

  void a();

 private:
  std::optional<AsyncCallback<std::string>> valueCallback_;
};

} // namespace facebook::react
