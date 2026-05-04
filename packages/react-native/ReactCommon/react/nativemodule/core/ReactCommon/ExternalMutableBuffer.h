/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsi/jsi.h>

namespace facebook::react {

/**
 * A jsi::MutableBuffer implementation that wraps externally-owned memory.
 * This class does not take ownership of the data pointer and will not
 * free it when destroyed.
 *
 * Use this when you need to create a jsi::ArrayBuffer from existing
 * memory that is managed elsewhere (e.g., Java ByteBuffer, NSData).
 */
class JSI_EXPORT ExternalMutableBuffer : public facebook::jsi::MutableBuffer {
 public:
  ExternalMutableBuffer(uint8_t* data, size_t size);

  [[nodiscard]] uint8_t* data() override;
  [[nodiscard]] size_t size() const override;

 private:
  uint8_t* data_{};
  size_t size_{};
};

} // namespace facebook::react
