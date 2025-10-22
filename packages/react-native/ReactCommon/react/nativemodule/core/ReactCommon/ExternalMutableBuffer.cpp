/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ExternalMutableBuffer.h"

namespace facebook::react {

ExternalMutableBuffer::ExternalMutableBuffer(uint8_t* data, size_t size)
    : data_(data), size_(size) {}

uint8_t* ExternalMutableBuffer::data() {
  return data_;
}
size_t ExternalMutableBuffer::size() const {
  return size_;
}

} // namespace facebook::react
