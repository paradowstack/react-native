/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TransformHelper.h"

#include <react/renderer/components/view/BaseViewProps.h>
#include <react/renderer/components/view/conversions.h>

#include "NativeArray.h"

using namespace facebook::jni;

namespace facebook::react {

namespace {
void processTransform(
    jni::alias_ref<jclass> /*unused*/,
    NativeArray* jTransforms,
    jni::alias_ref<jni::JArrayDouble> jResult,
    float viewWidth,
    float viewHeight,
    NativeArray* jTransformOrigin) {
  // The ContextContainer has no relevant content for transform parsing;
  // it is only needed to satisfy PropsParserContext's constructor.
  static ContextContainer contextContainer;

  // Create a fresh per-call context so concurrent calls are isolated.
  // calcMap points to a local DynamicPropertiesMap so that any calc()
  // values parsed inside fromRawValue are stored here, not in shared state.
  DynamicPropertiesMap calcExpressions;
  PropsParserContext context(0, contextContainer);
  context.calcMap = &calcExpressions;

  RawValue transformValue(jTransforms->getArray());
  Transform transform;
  fromRawValue(context, transformValue, transform);

  TransformOrigin transformOrigin;
  if (jTransformOrigin != nullptr) {
    RawValue transformOriginValue(jTransformOrigin->getArray());
    fromRawValue(context, transformOriginValue, transformOrigin);
  }

  auto resolver = DynamicResolver{
      calcExpressions, {{.frame = {{}, {viewWidth, viewHeight}}}, {}}};
  auto result =
      BaseViewProps::resolveTransform(resolver, transform, transformOrigin);

  // Convert from matrix of floats to double matrix
  constexpr size_t MatrixSize = std::tuple_size_v<decltype(result.matrix)>;
  std::array<double, MatrixSize> doubleTransform{};
  std::copy(
      result.matrix.begin(), result.matrix.end(), doubleTransform.begin());
  jResult->setRegion(0, MatrixSize, doubleTransform.data());
}

} // namespace

void TransformHelper::registerNatives() {
  javaClassLocal()->registerNatives({
      makeNativeMethod("nativeProcessTransform", processTransform),
  });
}

} // namespace facebook::react
