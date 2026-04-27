/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/NumericValue.h>

namespace facebook::react {

struct BoxShadow {
  bool operator==(const BoxShadow& other) const = default;

  LengthValue offsetX{};
  LengthValue offsetY{};
  LengthValue blurRadius{};
  LengthValue spreadDistance{};
  SharedColor color{};
  bool inset{};

#ifdef RN_SERIALIZABLE_STATE
  folly::dynamic toDynamic() const {
    folly::dynamic result = folly::dynamic::object();
    result["offsetX"] = offsetX.asFloat();
    result["offsetY"] = offsetY.asFloat();
    result["blurRadius"] = blurRadius.asFloat();
    result["spreadDistance"] = spreadDistance.asFloat();
    result["color"] = *color;
    result["inset"] = inset;
    return result;
  }
#endif
};

#ifdef RN_SERIALIZABLE_STATE
inline folly::dynamic toDynamic(const BoxShadow& boxShadow) {
  return boxShadow.toDynamic();
}
#endif

} // namespace facebook::react
