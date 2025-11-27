/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageBackground.h"

namespace facebook::react {

#ifdef RN_SERIALIZABLE_STATE
folly::dynamic ImageBackground::toDynamic() const {
  folly::dynamic result = folly::dynamic::object();
  result["type"] = "image";
  result["url"] = url;
  return result;
}
#endif

#if RN_DEBUG_STRING_CONVERTIBLE
void ImageBackground::toString(std::stringstream& ss) const {
  ss << "url(" << url << ")";
}
#endif

}; // namespace facebook::react
