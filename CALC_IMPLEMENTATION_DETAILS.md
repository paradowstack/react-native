# CSS `calc()` Support for React Native — Implementation Details

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Data Flow](#data-flow)
4. [Component Inventory](#component-inventory)
   - [CSSCalc — Expression Representation](#csscalc--expression-representation)
   - [CSSCalc Parser](#csscalc-parser)
   - [CalcExpressionPropertyID Enum](#calcexpressionpropertytid-enum)
   - [CalcExpressions Type Alias](#calcexpressions-type-alias)
   - [Yoga Dynamic Value Types](#yoga-dynamic-value-types)
   - [Unit::Dynamic / YGUnitDynamic](#unitdynamic--ygunitdynamic)
   - [StyleLength — Dynamic Extension](#stylelength--dynamic-extension)
   - [StyleSizeLength — Dynamic Extension](#stylesizelengthdynamic-extension)
   - [StyleValuePool — Dynamic Storage](#stylevaluepool--dynamic-storage)
   - [StyleValueHandle — Dynamic Type](#stylevaluehandle--dynamic-type)
   - [YGNodeStyle C API — Dynamic Setters](#ygnodestyle-c-api--dynamic-setters)
   - [YogaStylableProps — Calc Expression Building](#yogastylableprops--calc-expression-building)
   - [YogaLayoutableShadowNode — Calc Resolution](#yogalayoutableshadownode--calc-resolution)
   - [LayoutContext — Viewport Size](#layoutcontext--viewport-size)
   - [Platform Integration](#platform-integration)
5. [Supported Properties](#supported-properties)
6. [Supported CSS Units](#supported-css-units)
7. [Resolution Lifecycle](#resolution-lifecycle)
8. [Dirty-Marking Strategy](#dirty-marking-strategy)
9. [Design Decisions](#design-decisions)
10. [Known Limitations](#known-limitations)
11. [Test Coverage](#test-coverage)
12. [File Index](#file-index)

---

## Overview

This implementation adds CSS `calc()` support to React Native's layout system. It introduces a callback-based mechanism in Yoga where style values containing `calc()` expressions are stored as "dynamic" values and resolved just-in-time during layout via a callback into the React Native renderer.

**Key principle:** Yoga does not evaluate `calc()` expressions itself. It stores a function pointer + identifier, and at layout time calls back into the host (React Native) to resolve the value to a concrete point value.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        JS / React                               │
│   style={{ width: "calc(100% - 32px + 2vw)" }}                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │ RawProps
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  YogaStylableProps                               │
│                                                                 │
│  1. Parse "calc(...)" string → CSSCalc struct                   │
│  2. If points-only → yogaStyle.set*(StyleLength::points(px))    │
│     If percent-only → yogaStyle.set*(StyleLength::percent(%))   │
│     If mixed → yogaStyle.set*(StyleLength::dynamic(cb, id))     │
│                + store CSSCalc in calcExpressions[id]           │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     yoga::Style                                  │
│                                                                 │
│  StyleLength / StyleSizeLength with Unit::Dynamic               │
│  Payload union: { FloatOptional value | YGValueDynamicData }    │
│  Storage via StyleValuePool (callback as uint64 + id as uint32) │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼ (during layout)
┌─────────────────────────────────────────────────────────────────┐
│               StyleLength::resolve(ref, node)                   │
│                                                                 │
│  case Unit::Dynamic:                                            │
│    callback(node, id, {referenceLength}) → YGValue{pt, Point}  │
└──────────────────────────┬──────────────────────────────────────┘
                           │ callback
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│     YogaLayoutableShadowNode::yogaNodeCalcValueResolver         │
│                                                                 │
│  1. Get ShadowNode from yoga node context                       │
│  2. Look up CSSCalc in props.calcExpressions[id]               │
│  3. calc.resolve(ref, viewportW, viewportH)                    │
│  4. Return YGValue{resolved_float, YGUnitPoint}                 │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

### Parse-time (prop construction)

1. JS sets a style like `width: "calc(100% - 32px + 2vw)"`.
2. `YogaStylableProps` constructor calls `buildCalcExpressions()`.
3. The `APPLY_CALC_COMMON` macro parses the string via `parseCSSProperty<CSSCalc>()`.
4. **Optimization**: If the parsed `CSSCalc` is points-only (`px` component only) or percent-only, it's stored as a regular `StyleLength::points()` / `StyleLength::percent()` — no dynamic value needed.
5. **Mixed expressions**: Stored as `StyleLength::dynamic(&yogaNodeCalcValueResolver, propertyId)` in the yoga style, and the full `CSSCalc` struct is stored in `calcExpressions[propertyId]`.

### Layout-time (resolution)

1. `layoutTree()` sets `threadLocalLayoutContext = layoutContext` (which contains `viewportSize`).
2. `YGNodeCalculateLayout()` runs the Yoga layout algorithm.
3. When Yoga needs to resolve a dimension/margin/padding/position/gap, it calls `StyleLength::resolve(referenceLength, node)`.
4. For `Unit::Dynamic`, `resolve()` invokes `payload_.dynamic.callback(node, id, {referenceLength})`.
5. The callback (`yogaNodeCalcValueResolver`) extracts the `CSSCalc` from the ShadowNode's props and calls `CSSCalc::resolve(referenceLength, viewportWidth, viewportHeight)`.
6. Returns `YGValue{resolvedFloat, YGUnitPoint}`.

## Component Inventory

### CSSCalc — Expression Representation

**File:** `packages/react-native/ReactCommon/react/renderer/css/CSSCalc.h`

```cpp
struct CSSCalc {
  float px{0.0f};       // absolute pixel component
  float percent{0.0f};  // percentage component
  float vw{0.0f};       // viewport-width component
  float vh{0.0f};       // viewport-height component
  bool unitless{false};  // true if value is a plain number (e.g. from multiplication)

  constexpr auto operator==(const CSSCalc &rhs) const -> bool = default;

  // Arithmetic operators: +, -, *(scalar), /(scalar), unary -
  // resolve(percentRef, viewportWidth, viewportHeight) → float
  // Query methods: isUnitless(), isPointsOnly(), isPercentOnly(), isZero()
  // Factory methods: fromNumber(), fromPoints(), fromPercent(), fromVw(), fromVh(), fromLength()
};
```

**Resolution formula:**
```
result = px + (percent × percentRef × 0.01) + (vw × viewportWidth × 0.01) + (vh × viewportHeight × 0.01)
```

**Design notes:**
- Uses a linear decomposition: any `calc()` expression involving `+`, `-`, `*`, `/` with `px`, `%`, `vw`, `vh` units can be represented as four independent coefficients.
- Multiplication of two unit-bearing values (e.g. `10px * 20%`) is rejected at parse time — only `unit * number` or `number * unit` is valid per CSS spec.
- Division by zero returns `CSSCalc{}` (all zeros). CSS spec says infinity/NaN — minor spec deviation.

### CSSCalc Parser

**File:** `packages/react-native/ReactCommon/react/renderer/css/CSSCalc.h` (template specialization `CSSDataTypeParser<CSSCalc>`)

Recursive descent parser integrated into the existing `CSSValueParser` framework:

```
parseCalcExpression → parseAddSub → parseMulDiv → parseUnary → parsePrimary
                                                                     ↕
                                                          CSSNumber | CSSPercentage | CSSLength | CSSCalc (nested)
```

- **Entry**: `consumeFunctionBlock` — matches `calc(...)` function name (case-insensitive).
- **Nested calc**: `consumeSimpleBlock` handles parenthesized sub-expressions `(...)`.
- **Operator precedence**: `*` and `/` bind tighter than `+` and `-`.
- **Type checking**:
  - `+` / `-` require both operands to have the same "kind" (both unitless or both unit-bearing).
  - `*` requires exactly one operand to be unitless.
  - `/` requires the divisor to be unitless and non-zero.

### CalcExpressionPropertyID Enum

**File:** `packages/react-native/ReactCommon/react/renderer/components/view/primitives.h`

```cpp
enum class CalcExpressionPropertyID : uint8_t {
  Width, Height,
  MinWidth, MinHeight,
  MaxWidth, MaxHeight,
  FlexBasis,
  RowGap, ColumnGap, Gap,
  Left, Top, Right, Bottom, Start, End, InsetInline, InsetBlock, Inset,
  MarginLeft, MarginTop, MarginRight, MarginBottom,
  MarginStart, MarginEnd, MarginHorizontal, MarginVertical, MarginAll,
  PaddingLeft, PaddingTop, PaddingRight, PaddingBottom,
  PaddingStart, PaddingEnd, PaddingHorizontal, PaddingVertical, PaddingAll,
};
```

**37 entries total.** Each entry is a fixed `uint8_t` identifier that maps 1:1 to a Yoga style property slot. This ID is passed as `YGValueDynamicID` to the callback and used to look up the corresponding `CSSCalc` in the `calcExpressions` map.

### CalcExpressions Type Alias

**File:** `packages/react-native/ReactCommon/react/renderer/components/view/primitives.h`

```cpp
using CalcExpressions = std::unordered_map<CalcExpressionPropertyID, CSSCalc>;
```

Sparse storage — only entries with active `calc()` expressions are present. Empty for nodes without any `calc()` values.

### Yoga Dynamic Value Types

**File:** `packages/react-native/ReactCommon/yoga/yoga/YGValue.h`

```cpp
typedef uint8_t YGValueDynamicID;

typedef struct YGValueDynamicContext {
  float referenceLength;     // percentage reference (e.g. parent width)
} YGValueDynamicContext;

typedef YGValue (*YGValueDynamic)(
    YGNodeConstRef node,
    YGValueDynamicID id,
    YGValueDynamicContext context);

struct YGValueDynamicData {
  YGValueDynamic callback;   // function pointer
  YGValueDynamicID id;       // property identifier
};
```

**Naming convention:** Yoga uses "Dynamic" naming for its generic deferred-resolution mechanism. React Native renderer uses "Calc" naming for the specific `calc()` feature built on top.

### Unit::Dynamic / YGUnitDynamic

**Files:**
- `yoga/YGEnums.h` — declares `YGUnitDynamic` in the `YGUnit` C enum.
- `yoga/enums/Unit.h` — C++ scoped enum `Unit::Dynamic = YGUnitDynamic`.

Added as the 8th unit type alongside `Undefined`, `Point`, `Percent`, `Auto`, `MaxContent`, `FitContent`, `Stretch`.

### StyleLength — Dynamic Extension

**File:** `packages/react-native/ReactCommon/yoga/yoga/style/StyleLength.h`

Extended with:

- **Factory:** `static StyleLength dynamic(YGValueDynamic callback, YGValueDynamicID id)`
- **Query:** `constexpr bool isDynamic() const` — checks `unit_ == Unit::Dynamic`
- **Accessors:** `callback()` and `callbackId()` — return dynamic payload when `isDynamic()`
- **Resolution:** `resolve(float referenceLength, YGNodeConstRef node)`:
  - `Unit::Point` → returns `payload_.value`
  - `Unit::Percent` → returns `value * referenceLength * 0.01`
  - `Unit::Dynamic` → calls `payload_.dynamic.callback(node, id, {referenceLength})`, returns result value
  - Default → returns `FloatOptional{}`
- **Equality:** `operator==` compares `callback` pointer and `id` for dynamic values (not the float payload).
- **Internal storage:** Union `Payload { FloatOptional value; YGValueDynamicData dynamic; }` — reuses the same space for either a float or a callback+id pair.

### StyleSizeLength — Dynamic Extension

**File:** `packages/react-native/ReactCommon/yoga/yoga/style/StyleSizeLength.h`

Structurally identical extension to `StyleLength`, with the same `dynamic()` factory, `isDynamic()` query, `resolve()` with callback dispatch, and union payload. Adds `MaxContent`, `FitContent`, `Stretch` keyword support on top.

### StyleValuePool — Dynamic Storage

**File:** `packages/react-native/ReactCommon/yoga/yoga/style/StyleValuePool.h`

Extended with:

```cpp
void storeDynamic(StyleValueHandle& handle, YGValueDynamic callback, YGValueDynamicID id)
```

**Storage layout in SmallValueBuffer:**
- 64-bit slot: function pointer cast to `uint64_t` via `reinterpret_cast<uintptr_t>`
- 32-bit slot: callback ID as `uint32_t`
- Total: 12 bytes per dynamic value in the buffer

**Retrieval:**
```cpp
YGValueDynamic getDynamicCallback(StyleValueHandle handle) const
YGValueDynamicID getDynamicCallbackID(StyleValueHandle handle) const
```

**Note:** The function pointer cast (`reinterpret_cast<uintptr_t>`) is technically UB per the C++ standard but works on all target architectures (arm64, x86_64) where `sizeof(void(*)()) == sizeof(uintptr_t)`.

### StyleValueHandle — Dynamic Type

**File:** `packages/react-native/ReactCommon/yoga/yoga/style/StyleValueHandle.h`

Extended the `Type` enum with `Dynamic` (value 6) and added `isDynamic()` query method. The handle uses 16-bit packed representation:
- Bits 0-2: Type (7 values: Undefined, Point, Percent, Number, Auto, Keyword, Dynamic)
- Bit 3: isValueIndexed flag
- Bits 4-15: value (12-bit index or inline value)

### YGNodeStyle C API — Dynamic Setters

**File:** `packages/react-native/ReactCommon/yoga/yoga/YGNodeStyle.h` / `.cpp`

Added 12 `*Dynamic` setter functions:

| Function | Style Type | Index Type |
|----------|-----------|------------|
| `YGNodeStyleSetFlexBasisDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetWidthDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetHeightDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetMinWidthDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetMinHeightDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetMaxWidthDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetMaxHeightDynamic` | `StyleSizeLength` | — |
| `YGNodeStyleSetPositionDynamic` | `StyleLength` | `YGEdge` |
| `YGNodeStyleSetMarginDynamic` | `StyleLength` | `YGEdge` |
| `YGNodeStyleSetPaddingDynamic` | `StyleLength` | `YGEdge` |
| `YGNodeStyleSetBorderDynamic` | `StyleLength` | `YGEdge` |
| `YGNodeStyleSetGapDynamic` | `StyleLength` | `YGGutter` |

All follow the same pattern:
```cpp
void YGNodeStyleSetPositionDynamic(YGNodeRef node, YGEdge edge,
    YGValueDynamic dynamicValue, uint8_t dynamicId) {
  updateStyle<&Style::position, &Style::setPosition>(
      node, scopedEnum(edge), StyleLength::dynamic(dynamicValue, dynamicId));
}
```

### YogaStylableProps — Calc Expression Building

**File:** `packages/react-native/ReactCommon/react/renderer/components/view/YogaStylableProps.cpp`

#### Constructor Flow

```cpp
YogaStylableProps::YogaStylableProps(context, sourceProps, rawProps, filterObjectKeys) {
  initialize(context, sourceProps, rawProps, filterObjectKeys);
  yogaStyle = enableCppPropsIteratorSetter()
      ? sourceProps.yogaStyle
      : convertRawProp(context, rawProps, sourceProps.yogaStyle);
  calcExpressions = buildCalcExpressions(context, rawProps, sourceProps.calcExpressions);
  if (!enableCppPropsIteratorSetter()) {
    convertRawPropAliases(context, sourceProps, rawProps);
  }
}
```

#### Core Macro: APPLY_CALC_COMMON

```cpp
#define APPLY_CALC_COMMON(fieldName, key, setPoints, setPercent, setDynamic)
  // 1. Look up raw prop by name
  // 2. If it's a string, try parseCSSProperty<CSSCalc>()
  // 3. If CSSCalc result:
  //    - isPointsOnly()  → setPoints(calc.px)
  //    - isPercentOnly() → setPercent(calc.percent)
  //    - mixed           → setDynamic(id); calcExpressions[key] = calc
  // 4. If was NOT a calc expression and old entry exists → erase it
```

**Optimization path:** Simple `calc(50px)` or `calc(50%)` values are NOT stored as dynamic — they're collapsed to regular point/percent values. Only mixed expressions (e.g. `calc(100% - 32px)`, `calc(50vw + 10px)`) become dynamic values.

#### buildCalcExpressions()

Invokes the APPLY_CALC macros for all supported properties:
- Dimensions: width, height, minWidth, minHeight, maxWidth, maxHeight
- FlexBasis
- Gap: rowGap, columnGap, gap
- Position edges: left, top, right, bottom, start, end, insetInline, insetBlock, inset
- Margin edges: marginLeft/Top/Right/Bottom/Start/End/Horizontal/Vertical/All
- Padding edges: paddingLeft/Top/Right/Bottom/Start/End/Horizontal/Vertical/All

#### Free Function: yogaNodeCalcValueResolver

```cpp
YGValue yogaNodeCalcValueResolver(YGNodeConstRef yogaNode,
    YGValueDynamicID id, YGValueDynamicContext context) {
  return YogaLayoutableShadowNode::yogaNodeCalcValueResolver(yogaNode, id, context);
}
```

This free function (with C linkage-compatible signature) is what gets stored in the `StyleLength::dynamic()` payload. It delegates to the static method on `YogaLayoutableShadowNode`.

### YogaLayoutableShadowNode — Calc Resolution

**File:** `packages/react-native/ReactCommon/react/renderer/components/view/YogaLayoutableShadowNode.cpp`

#### Static Resolver Method

```cpp
YGValue YogaLayoutableShadowNode::yogaNodeCalcValueResolver(
    YGNodeConstRef yogaNode, YGValueDynamicID id, YGValueDynamicContext context) {
  if (!yogaNode) return {};

  auto& node = shadowNodeFromContext(yogaNode);
  auto& props = static_cast<const YogaStylableProps&>(*node.props_);
  auto key = static_cast<CalcExpressionPropertyID>(id);

  if (!props.calcExpressions.contains(key)) return {};

  auto& calc = props.calcExpressions.at(key);
  return YGValue(
      calc.resolve(context.referenceLength,
                   threadLocalLayoutContext.viewportSize.width,
                   threadLocalLayoutContext.viewportSize.height),
      YGUnitPoint);
}
```

**Key dependencies:**
- `shadowNodeFromContext()` — retrieves the `YogaLayoutableShadowNode` from the yoga node's user context pointer.
- `threadLocalLayoutContext` — thread-local variable set before `YGNodeCalculateLayout()`, provides viewport dimensions.

#### Dirty-Marking in updateYogaProps()

```cpp
void YogaLayoutableShadowNode::updateYogaProps(
    const CalcExpressions& previousCalcExpressions) {
  auto& props = static_cast<const YogaStylableProps&>(*props_);
  auto styleResult = applyAliasedProps(props.yogaStyle, props);

  if (!YGNodeIsDirty(&yogaNode_) &&
      (props.calcExpressions != previousCalcExpressions ||
       styleResult != yogaNode_.style())) {
    yogaNode_.setDirty(true);
  }
  yogaNode_.setStyle(styleResult);
}
```

- **Initial construction:** Called with default `{}` → compares against empty map → dirty if any calc expressions exist.
- **Clone (prop update):** Called with `sourceProps.calcExpressions` → only marks dirty if expressions actually changed.

### LayoutContext — Viewport Size

**File:** `packages/react-native/ReactCommon/react/renderer/core/LayoutContext.h`

```cpp
struct LayoutContext {
  Float pointScaleFactor{1.0};
  std::vector<const LayoutableShadowNode*>* affectedNodes{};
  bool swapLeftAndRightInRTL{false};
  Float fontSizeMultiplier{1.0};
  Point viewportOffset{};
  Size viewportSize{};           // ← used for vw/vh resolution
};
```

`viewportSize` is set by platform code:
- **Android** (`SurfaceHandlerBinding.cpp`): `context.viewportSize = {maxWidth, maxHeight}`
- **iOS** (`RCTFabricSurface.mm`): `layoutContext.viewportSize = layoutConstraints.maximumSize`

Stored in `thread_local LayoutContext threadLocalLayoutContext` before layout.

### Platform Integration

#### Android — `YogaUnit.java`
```java
public enum YogaUnit {
  UNDEFINED(0), POINT(1), PERCENT(2), AUTO(3),
  MAX_CONTENT(4), FIT_CONTENT(5), STRETCH(6), DYNAMIC(7);
}
```

#### iOS Paper — `RCTLayout.m`
```objc
case YGUnitDynamic: return RCTCoreGraphicsFloatFromYogaFloat(YGUndefined);
```
Returns NaN for Paper renderer (which can't resolve calc expressions).

#### Serialization — `graphicsConversions.h`
```cpp
case YGUnitDynamic: return "calc(dynamic)";
```
Non-round-trippable placeholder since `YGValue` doesn't carry the original expression.

## Supported Properties

| Category | Properties |
|----------|-----------|
| **Dimensions** | `width`, `height` |
| **Min dimensions** | `minWidth`, `minHeight` |
| **Max dimensions** | `maxWidth`, `maxHeight` |
| **Flex** | `flexBasis` |
| **Gap** | `rowGap`, `columnGap`, `gap` |
| **Position (inset)** | `left`, `top`, `right`, `bottom`, `start`, `end`, `insetInline`, `insetBlock`, `inset` |
| **Margin** | `marginLeft`, `marginTop`, `marginRight`, `marginBottom`, `marginStart`, `marginEnd`, `marginHorizontal`, `marginVertical`, `margin` |
| **Padding** | `paddingLeft`, `paddingTop`, `paddingRight`, `paddingBottom`, `paddingStart`, `paddingEnd`, `paddingHorizontal`, `paddingVertical`, `padding` |
| **Border** *(API only)* | `YGNodeStyleSetBorderDynamic` exists but no `CalcExpressionPropertyID` entries — not wired end-to-end yet |

## Supported CSS Units

| Unit | Example | Resolution |
|------|---------|------------|
| `px` | `calc(100px)` | Direct pixel value |
| `%` | `calc(50%)` | `value × referenceLength × 0.01` |
| `vw` | `calc(10vw)` | `value × viewportWidth × 0.01` |
| `vh` | `calc(5vh)` | `value × viewportHeight × 0.01` |

**Supported operations:**
- `+` and `-` between compatible types (unit + unit, or number + number)
- `*` between a unit value and a number
- `/` by a non-zero number
- Unary `-` and `+`
- Nested parentheses `(...)` and nested `calc(...)`

## Resolution Lifecycle

```
1. ShadowTree Commit
   └→ YogaStylableProps constructor
       └→ buildCalcExpressions()
           └→ APPLY_CALC_COMMON macros
               ├→ parseCSSProperty<CSSCalc>(string)
               ├→ [points-only] → yogaStyle.set*(StyleLength::points(px))
               ├→ [percent-only] → yogaStyle.set*(StyleLength::percent(%))
               └→ [mixed] → yogaStyle.set*(StyleLength::dynamic(cb, id))
                           + calcExpressions[id] = CSSCalc{px, %, vw, vh}

2. Layout Pass
   └→ layoutTree(layoutContext, constraints)
       ├→ threadLocalLayoutContext = layoutContext  // stores viewport size
       ├→ configureYogaTree(...)
       └→ YGNodeCalculateLayout(...)
           └→ (Yoga internal) needs dimension/margin/padding/position/gap value
               └→ StyleLength::resolve(referenceLength, node)
                   └→ [Unit::Dynamic] callback(node, id, {referenceLength})
                       └→ yogaNodeCalcValueResolver(node, id, context)
                           ├→ shadowNodeFromContext(node) → ShadowNode
                           ├→ props.calcExpressions.at(id) → CSSCalc
                           └→ calc.resolve(ref, viewportW, viewportH) → float
```

## Dirty-Marking Strategy

The yoga node is marked dirty only when calc expressions actually change between revisions:

| Scenario | `previousCalcExpressions` | Comparison | Result |
|----------|--------------------------|------------|--------|
| Initial construction | `{}` (empty) | `props.calcExpressions != {}` | Dirty if any calc exists |
| Clone, no prop change | N/A (updateYogaProps not called) | — | Inherits dirty flag |
| Clone, props changed | `sourceProps.calcExpressions` | `props.calcExpressions != previous` | Dirty only if calc changed |
| Clone, calc removed | `sourceProps.calcExpressions` (non-empty) | `props.calcExpressions` (empty) `!=` previous | Dirty ✓ |
| Clone, calc unchanged | `sourceProps.calcExpressions` | Equal | Not dirtied by calc |

## Design Decisions

### 1. Callback-based resolution vs. storing expressions in Yoga

**Decision:** Yoga stores a function pointer + ID, not the expression itself.

**Rationale:** Keeps Yoga engine clean and generic. `calc()` is a React Native concern; Yoga just provides the hook for "I need this value resolved at layout time." Other hosts could use the same `Unit::Dynamic` mechanism for entirely different purposes.

### 2. Linear decomposition of calc expressions

**Decision:** Represent `calc()` as four floats `{px, percent, vw, vh}`.

**Rationale:** CSS `calc()` with standard arithmetic operators on length values always decomposes linearly. This avoids storing an AST and enables O(1) resolution. The parser does the heavy lifting; resolution is a single multiply-add.

### 3. Points-only / percent-only optimization

**Decision:** `calc(50px)` is stored as `StyleLength::points(50)`, not as a dynamic value.

**Rationale:** Avoids the overhead of callback dispatch for trivially resolvable values. A `calc()` wrapping a single unit is semantically equivalent to a plain value.

### 4. thread_local for viewport dimensions

**Decision:** Store `LayoutContext` (containing `viewportSize`) in a `thread_local` variable.

**Rationale:** The `YGValueDynamic` callback signature only receives `(node, id, context)` where `context` is `{referenceLength}`. Viewport dimensions can't be threaded through this. A thread-local is safe because layout runs on a single thread and is set just before `YGNodeCalculateLayout`.

### 5. Naming: "Dynamic" (Yoga) vs "Calc" (RN)

**Decision:** Yoga layer uses "Dynamic" naming; RN renderer layer uses "Calc" naming.

**Rationale:** Clean separation of concerns. Yoga's mechanism is generic and could serve other deferred-resolution use cases. RN's usage is specifically for CSS `calc()`.

### 6. `std::unordered_map` for calcExpressions

**Decision:** Use `std::unordered_map<CalcExpressionPropertyID, CSSCalc>` instead of a fixed-size array.

**Rationale:** Most nodes won't have any `calc()` expressions, and those that do will typically have 1-3. A map avoids 37 × sizeof(CSSCalc) = ~740 bytes per node for the common case of zero calc expressions. The map is empty (zero heap allocation) when unused.

## Known Limitations

1. **Border width calc() not wired end-to-end.** `YGNodeStyleSetBorderDynamic` exists in the C API but `CalcExpressionPropertyID` has no `BorderLeft`..`BorderAll` entries, and `buildCalcExpressions()` doesn't process border properties. Border `calc()` will need follow-up work.

2. **Function pointer storage is technically UB.** `StyleValuePool::storeDynamic()` casts `YGValueDynamic` (function pointer) to `uintptr_t` to `uint64_t` for compact storage. This works on arm64/x86_64 but is not guaranteed by the C++ standard.

3. **`YGValue::operator==` returns `false` for `YGUnitDynamic`.** By design — `YGValue` only carries `{float, YGUnit}` and can't meaningfully compare dynamic values. The real comparison happens at `StyleLength`/`StyleSizeLength` level using callback pointer + id.

4. **Division by zero returns zero, not infinity.** `CSSCalc::operator/` returns `CSSCalc{}` when dividing by zero. The CSS spec says the result should be `infinity` or `NaN`.

5. **iOS `_updateLayoutContext` may not set `viewportSize`.** The `setMinimumSize:maximumSize:viewportOffset:` method does set it, but `_updateLayoutContext` alone doesn't. This means if only `_updateLayoutContext` is called (e.g., on font scale change), `viewportSize` retains its previous value from `setMinimumSize:...`.

6. **No JNI/Java/JS API for setting calc from native.** The C++ pipeline works end-to-end from RawProps, but there's no Java `YogaNode.setCalc()` or JS API to programmatically create calc values outside of style strings.

## Test Coverage

**File:** `packages/react-native/ReactCommon/react/renderer/css/tests/CSSCalcTest.cpp`

**50 test cases** covering:

| Category | Tests |
|----------|-------|
| **Basic parsing** | `simple_pixel_value`, `simple_percentage_value`, `simple_vw_value`, `simple_vh_value` |
| **Arithmetic** | `addition_same_units`, `subtraction_same_units`, `mixed_units_addition`, `mixed_units_complex` |
| **Multiplication** | `multiplication_by_number`, `number_times_unit`, `chained_unitless_products_then_length`, `unitless_division_then_length_multiplication` |
| **Division** | `division_by_number`, `division_by_zero`, `division_by_zero_operator` |
| **Precedence** | `complex_expression_with_precedence`, `operator_precedence_mul_before_add` |
| **Nesting** | `nested_parentheses`, `nested_calc` |
| **Unary** | `negative_values`, `unary_plus`, `negation_operator`, `negation_preserves_unitless` |
| **Resolution** | `resolve_simple_percentage`, `resolve_mixed_units`, `resolve_with_viewport_units`, `resolve_all_units` |
| **Validation** | `invalid_expression_empty`, `invalid_multiplication_of_units`, `invalid_division_by_unit`, `invalid_addition_of_number_and_length`, `invalid_subtraction_of_percent_and_number`, `invalid_wrong_function` |
| **Misc** | `whitespace_handling`, `case_insensitive`, `equality` |
| **Operators** | `addition_operator`, `subtraction_operator`, `multiplication_operator`, `division_operator` |
| **Queries** | `is_unitless`, `is_points_only`, `is_percent_only`, `is_zero`, `is_zero_unitless` |
| **Factories** | `from_points`, `from_percent`, `from_vw`, `from_vh`, `from_length` |

## File Index

| File | Purpose |
|------|---------|
| `react/renderer/css/CSSCalc.h` | `CSSCalc` struct + recursive descent parser |
| `react/renderer/css/tests/CSSCalcTest.cpp` | 50 unit tests for CSSCalc parsing and resolution |
| `react/renderer/components/view/primitives.h` | `CalcExpressionPropertyID` enum + `CalcExpressions` type alias |
| `react/renderer/components/view/YogaStylableProps.h` | `calcExpressions` field + `buildCalcExpressions()` declaration |
| `react/renderer/components/view/YogaStylableProps.cpp` | `APPLY_CALC_*` macros, `buildCalcExpressions()`, `yogaNodeCalcValueResolver` free function |
| `react/renderer/components/view/YogaLayoutableShadowNode.h` | `yogaNodeCalcValueResolver` static method, `updateYogaProps` with calc comparison |
| `react/renderer/components/view/YogaLayoutableShadowNode.cpp` | Calc resolver implementation, dirty-marking logic, `threadLocalLayoutContext` |
| `react/renderer/core/LayoutContext.h` | `viewportSize` field in `LayoutContext` |
| `react/renderer/core/graphicsConversions.h` | `YGUnitDynamic` → `"calc(dynamic)"` serialization |
| `react/renderer/components/view/conversions.h` | `fromRawValue` for `std::optional<CSSCalc>` |
| `yoga/YGValue.h` | `YGValueDynamic`, `YGValueDynamicID`, `YGValueDynamicContext`, `YGValueDynamicData` typedefs |
| `yoga/YGEnums.h` | `YGUnitDynamic` enum value |
| `yoga/enums/Unit.h` | `Unit::Dynamic` C++ scoped enum |
| `yoga/style/StyleLength.h` | `StyleLength::dynamic()`, `resolve()` with callback dispatch, union payload |
| `yoga/style/StyleSizeLength.h` | `StyleSizeLength::dynamic()`, same pattern as StyleLength |
| `yoga/style/StyleValuePool.h` | `storeDynamic()`, `getDynamicCallback()`, `getDynamicCallbackID()` |
| `yoga/style/StyleValueHandle.h` | `Type::Dynamic` enum value, `isDynamic()` query |
| `yoga/YGNodeStyle.h` | 12 `YGNodeStyleSet*Dynamic` C API declarations |
| `yoga/YGNodeStyle.cpp` | 12 `YGNodeStyleSet*Dynamic` C API implementations |
| `React/Views/RCTLayout.m` | `YGUnitDynamic` handling for Paper renderer (returns NaN) |
| `React/Fabric/Surface/RCTFabricSurface.mm` | `viewportSize` setup for iOS |
| `ReactAndroid/.../SurfaceHandlerBinding.cpp` | `viewportSize` setup for Android |
| `ReactAndroid/.../YogaUnit.java` | `DYNAMIC(7)` enum value for Java |
