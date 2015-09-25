// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_H_
#define V8_OBJECTS_H_

#include <iosfwd>

#include "src/allocation.h"
#include "src/assert-scope.h"
#include "src/bailout-reason.h"
#include "src/base/bits.h"
#include "src/base/smart-pointers.h"
#include "src/builtins.h"
#include "src/checks.h"
#include "src/elements-kind.h"
#include "src/field-index.h"
#include "src/flags.h"
#include "src/list.h"
#include "src/property-details.h"
#include "src/unicode.h"
#include "src/unicode-decoder.h"
#include "src/zone.h"

#if V8_TARGET_ARCH_ARM
#include "src/arm/constants-arm.h"  // NOLINT
#elif V8_TARGET_ARCH_ARM64
#include "src/arm64/constants-arm64.h"  // NOLINT
#elif V8_TARGET_ARCH_MIPS
#include "src/mips/constants-mips.h"  // NOLINT
#elif V8_TARGET_ARCH_MIPS64
#include "src/mips64/constants-mips64.h"  // NOLINT
#elif V8_TARGET_ARCH_PPC
#include "src/ppc/constants-ppc.h"  // NOLINT
#endif


//
// Most object types in the V8 JavaScript are described in this file.
//
// Inheritance hierarchy:
// - Object
//   - Smi          (immediate small integer)
//   - HeapObject   (superclass for everything allocated in the heap)
//     - JSReceiver  (suitable for property access)
//       - JSObject
//         - JSArray
//         - JSArrayBuffer
//         - JSArrayBufferView
//           - JSTypedArray
//           - JSDataView
//         - JSCollection
//           - JSSet
//           - JSMap
//         - JSSetIterator
//         - JSMapIterator
//         - JSWeakCollection
//           - JSWeakMap
//           - JSWeakSet
//         - JSRegExp
//         - JSFunction
//         - JSGeneratorObject
//         - JSModule
//         - GlobalObject
//           - JSGlobalObject
//           - JSBuiltinsObject
//         - JSGlobalProxy
//         - JSValue
//           - JSDate
//         - JSMessageObject
//       - JSProxy
//         - JSFunctionProxy
//     - FixedArrayBase
//       - ByteArray
//       - BytecodeArray
//       - FixedArray
//         - DescriptorArray
//         - HashTable
//           - Dictionary
//           - StringTable
//           - CompilationCacheTable
//           - CodeCacheHashTable
//           - MapCache
//         - OrderedHashTable
//           - OrderedHashSet
//           - OrderedHashMap
//         - Context
//         - TypeFeedbackVector
//         - ScopeInfo
//         - TransitionArray
//         - ScriptContextTable
//         - WeakFixedArray
//       - FixedDoubleArray
//     - Name
//       - String
//         - SeqString
//           - SeqOneByteString
//           - SeqTwoByteString
//         - SlicedString
//         - ConsString
//         - ExternalString
//           - ExternalOneByteString
//           - ExternalTwoByteString
//         - InternalizedString
//           - SeqInternalizedString
//             - SeqOneByteInternalizedString
//             - SeqTwoByteInternalizedString
//           - ConsInternalizedString
//           - ExternalInternalizedString
//             - ExternalOneByteInternalizedString
//             - ExternalTwoByteInternalizedString
//       - Symbol
//     - HeapNumber
//     - Simd128Value
//       - Float32x4
//       - Int32x4
//       - Uint32x4
//       - Bool32x4
//       - Int16x8
//       - Uint16x8
//       - Bool16x8
//       - Int8x16
//       - Uint8x16
//       - Bool8x16
//     - Cell
//     - PropertyCell
//     - Code
//     - Map
//     - Oddball
//     - Foreign
//     - SharedFunctionInfo
//     - Struct
//       - Box
//       - AccessorInfo
//         - ExecutableAccessorInfo
//       - AccessorPair
//       - AccessCheckInfo
//       - InterceptorInfo
//       - CallHandlerInfo
//       - TemplateInfo
//         - FunctionTemplateInfo
//         - ObjectTemplateInfo
//       - Script
//       - TypeSwitchInfo
//       - DebugInfo
//       - BreakPointInfo
//       - CodeCache
//       - PrototypeInfo
//     - WeakCell
//
// Formats of Object*:
//  Smi:        [31 bit signed int] 0
//  HeapObject: [32 bit direct pointer] (4 byte aligned) | 01

namespace v8 {
namespace internal {

enum KeyedAccessStoreMode {
  STANDARD_STORE,
  STORE_TRANSITION_TO_OBJECT,
  STORE_TRANSITION_TO_DOUBLE,
  STORE_AND_GROW_NO_TRANSITION,
  STORE_AND_GROW_TRANSITION_TO_OBJECT,
  STORE_AND_GROW_TRANSITION_TO_DOUBLE,
  STORE_NO_TRANSITION_IGNORE_OUT_OF_BOUNDS,
  STORE_NO_TRANSITION_HANDLE_COW
};


// Valid hints for the abstract operation ToPrimitive,
// implemented according to ES6, section 7.1.1.
enum class ToPrimitiveHint { kDefault, kNumber, kString };


// Valid hints for the abstract operation OrdinaryToPrimitive,
// implemented according to ES6, section 7.1.1.
enum class OrdinaryToPrimitiveHint { kNumber, kString };


enum TypeofMode { INSIDE_TYPEOF, NOT_INSIDE_TYPEOF };


enum MutableMode {
  MUTABLE,
  IMMUTABLE
};


enum ExternalArrayType {
  kExternalInt8Array = 1,
  kExternalUint8Array,
  kExternalInt16Array,
  kExternalUint16Array,
  kExternalInt32Array,
  kExternalUint32Array,
  kExternalFloat32Array,
  kExternalFloat64Array,
  kExternalUint8ClampedArray,
};


static inline bool IsTransitionStoreMode(KeyedAccessStoreMode store_mode) {
  return store_mode == STORE_TRANSITION_TO_OBJECT ||
         store_mode == STORE_TRANSITION_TO_DOUBLE ||
         store_mode == STORE_AND_GROW_TRANSITION_TO_OBJECT ||
         store_mode == STORE_AND_GROW_TRANSITION_TO_DOUBLE;
}


static inline KeyedAccessStoreMode GetNonTransitioningStoreMode(
    KeyedAccessStoreMode store_mode) {
  if (store_mode >= STORE_NO_TRANSITION_IGNORE_OUT_OF_BOUNDS) {
    return store_mode;
  }
  if (store_mode >= STORE_AND_GROW_NO_TRANSITION) {
    return STORE_AND_GROW_NO_TRANSITION;
  }
  return STANDARD_STORE;
}


static inline bool IsGrowStoreMode(KeyedAccessStoreMode store_mode) {
  return store_mode >= STORE_AND_GROW_NO_TRANSITION &&
         store_mode <= STORE_AND_GROW_TRANSITION_TO_DOUBLE;
}


enum IcCheckType { ELEMENT, PROPERTY };


// SKIP_WRITE_BARRIER skips the write barrier.
// UPDATE_WEAK_WRITE_BARRIER skips the marking part of the write barrier and
// only performs the generational part.
// UPDATE_WRITE_BARRIER is doing the full barrier, marking and generational.
enum WriteBarrierMode {
  SKIP_WRITE_BARRIER,
  UPDATE_WEAK_WRITE_BARRIER,
  UPDATE_WRITE_BARRIER
};


// Indicates whether a value can be loaded as a constant.
enum StoreMode { ALLOW_IN_DESCRIPTOR, FORCE_FIELD };


// PropertyNormalizationMode is used to specify whether to keep
// inobject properties when normalizing properties of a JSObject.
enum PropertyNormalizationMode {
  CLEAR_INOBJECT_PROPERTIES,
  KEEP_INOBJECT_PROPERTIES
};


// Indicates how aggressively the prototype should be optimized. FAST_PROTOTYPE
// will give the fastest result by tailoring the map to the prototype, but that
// will cause polymorphism with other objects. REGULAR_PROTOTYPE is to be used
// (at least for now) when dynamically modifying the prototype chain of an
// object using __proto__ or Object.setPrototypeOf.
enum PrototypeOptimizationMode { REGULAR_PROTOTYPE, FAST_PROTOTYPE };


// Indicates whether transitions can be added to a source map or not.
enum TransitionFlag {
  INSERT_TRANSITION,
  OMIT_TRANSITION
};


// Indicates whether the transition is simple: the target map of the transition
// either extends the current map with a new property, or it modifies the
// property that was added last to the current map.
enum SimpleTransitionFlag {
  SIMPLE_PROPERTY_TRANSITION,
  PROPERTY_TRANSITION,
  SPECIAL_TRANSITION
};


// Indicates whether we are only interested in the descriptors of a particular
// map, or in all descriptors in the descriptor array.
enum DescriptorFlag {
  ALL_DESCRIPTORS,
  OWN_DESCRIPTORS
};

// The GC maintains a bit of information, the MarkingParity, which toggles
// from odd to even and back every time marking is completed. Incremental
// marking can visit an object twice during a marking phase, so algorithms that
// that piggy-back on marking can use the parity to ensure that they only
// perform an operation on an object once per marking phase: they record the
// MarkingParity when they visit an object, and only re-visit the object when it
// is marked again and the MarkingParity changes.
enum MarkingParity {
  NO_MARKING_PARITY,
  ODD_MARKING_PARITY,
  EVEN_MARKING_PARITY
};

// ICs store extra state in a Code object. The default extra state is
// kNoExtraICState.
typedef int ExtraICState;
static const ExtraICState kNoExtraICState = 0;

// Instance size sentinel for objects of variable size.
const int kVariableSizeSentinel = 0;

// We may store the unsigned bit field as signed Smi value and do not
// use the sign bit.
const int kStubMajorKeyBits = 7;
const int kStubMinorKeyBits = kSmiValueSize - kStubMajorKeyBits - 1;

// All Maps have a field instance_type containing a InstanceType.
// It describes the type of the instances.
//
// As an example, a JavaScript object is a heap object and its map
// instance_type is JS_OBJECT_TYPE.
//
// The names of the string instance types are intended to systematically
// mirror their encoding in the instance_type field of the map.  The default
// encoding is considered TWO_BYTE.  It is not mentioned in the name.  ONE_BYTE
// encoding is mentioned explicitly in the name.  Likewise, the default
// representation is considered sequential.  It is not mentioned in the
// name.  The other representations (e.g. CONS, EXTERNAL) are explicitly
// mentioned.  Finally, the string is either a STRING_TYPE (if it is a normal
// string) or a INTERNALIZED_STRING_TYPE (if it is a internalized string).
//
// NOTE: The following things are some that depend on the string types having
// instance_types that are less than those of all other types:
// HeapObject::Size, HeapObject::IterateBody, the typeof operator, and
// Object::IsString.
//
// NOTE: Everything following JS_VALUE_TYPE is considered a
// JSObject for GC purposes. The first four entries here have typeof
// 'object', whereas JS_FUNCTION_TYPE has typeof 'function'.
#define INSTANCE_TYPE_LIST(V)                                   \
  V(STRING_TYPE)                                                \
  V(ONE_BYTE_STRING_TYPE)                                       \
  V(CONS_STRING_TYPE)                                           \
  V(CONS_ONE_BYTE_STRING_TYPE)                                  \
  V(SLICED_STRING_TYPE)                                         \
  V(SLICED_ONE_BYTE_STRING_TYPE)                                \
  V(EXTERNAL_STRING_TYPE)                                       \
  V(EXTERNAL_ONE_BYTE_STRING_TYPE)                              \
  V(EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE)                    \
  V(SHORT_EXTERNAL_STRING_TYPE)                                 \
  V(SHORT_EXTERNAL_ONE_BYTE_STRING_TYPE)                        \
  V(SHORT_EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE)              \
                                                                \
  V(INTERNALIZED_STRING_TYPE)                                   \
  V(ONE_BYTE_INTERNALIZED_STRING_TYPE)                          \
  V(EXTERNAL_INTERNALIZED_STRING_TYPE)                          \
  V(EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE)                 \
  V(EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE)       \
  V(SHORT_EXTERNAL_INTERNALIZED_STRING_TYPE)                    \
  V(SHORT_EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE)           \
  V(SHORT_EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE) \
                                                                \
  V(SYMBOL_TYPE)                                                \
  V(SIMD128_VALUE_TYPE)                                         \
                                                                \
  V(MAP_TYPE)                                                   \
  V(CODE_TYPE)                                                  \
  V(ODDBALL_TYPE)                                               \
  V(CELL_TYPE)                                                  \
  V(PROPERTY_CELL_TYPE)                                         \
                                                                \
  V(HEAP_NUMBER_TYPE)                                           \
  V(MUTABLE_HEAP_NUMBER_TYPE)                                   \
  V(FOREIGN_TYPE)                                               \
  V(BYTE_ARRAY_TYPE)                                            \
  V(BYTECODE_ARRAY_TYPE)                                        \
  V(FREE_SPACE_TYPE)                                            \
                                                                \
  V(FIXED_INT8_ARRAY_TYPE)                                      \
  V(FIXED_UINT8_ARRAY_TYPE)                                     \
  V(FIXED_INT16_ARRAY_TYPE)                                     \
  V(FIXED_UINT16_ARRAY_TYPE)                                    \
  V(FIXED_INT32_ARRAY_TYPE)                                     \
  V(FIXED_UINT32_ARRAY_TYPE)                                    \
  V(FIXED_FLOAT32_ARRAY_TYPE)                                   \
  V(FIXED_FLOAT64_ARRAY_TYPE)                                   \
  V(FIXED_UINT8_CLAMPED_ARRAY_TYPE)                             \
                                                                \
  V(FILLER_TYPE)                                                \
                                                                \
  V(DECLARED_ACCESSOR_DESCRIPTOR_TYPE)                          \
  V(DECLARED_ACCESSOR_INFO_TYPE)                                \
  V(EXECUTABLE_ACCESSOR_INFO_TYPE)                              \
  V(ACCESSOR_PAIR_TYPE)                                         \
  V(ACCESS_CHECK_INFO_TYPE)                                     \
  V(INTERCEPTOR_INFO_TYPE)                                      \
  V(CALL_HANDLER_INFO_TYPE)                                     \
  V(FUNCTION_TEMPLATE_INFO_TYPE)                                \
  V(OBJECT_TEMPLATE_INFO_TYPE)                                  \
  V(SIGNATURE_INFO_TYPE)                                        \
  V(TYPE_SWITCH_INFO_TYPE)                                      \
  V(ALLOCATION_MEMENTO_TYPE)                                    \
  V(ALLOCATION_SITE_TYPE)                                       \
  V(SCRIPT_TYPE)                                                \
  V(CODE_CACHE_TYPE)                                            \
  V(POLYMORPHIC_CODE_CACHE_TYPE)                                \
  V(TYPE_FEEDBACK_INFO_TYPE)                                    \
  V(ALIASED_ARGUMENTS_ENTRY_TYPE)                               \
  V(BOX_TYPE)                                                   \
  V(PROTOTYPE_INFO_TYPE)                                        \
  V(SLOPPY_BLOCK_WITH_EVAL_CONTEXT_EXTENSION_TYPE)              \
                                                                \
  V(FIXED_ARRAY_TYPE)                                           \
  V(FIXED_DOUBLE_ARRAY_TYPE)                                    \
  V(SHARED_FUNCTION_INFO_TYPE)                                  \
  V(WEAK_CELL_TYPE)                                             \
                                                                \
  V(JS_MESSAGE_OBJECT_TYPE)                                     \
                                                                \
  V(JS_VALUE_TYPE)                                              \
  V(JS_DATE_TYPE)                                               \
  V(JS_OBJECT_TYPE)                                             \
  V(JS_CONTEXT_EXTENSION_OBJECT_TYPE)                           \
  V(JS_GENERATOR_OBJECT_TYPE)                                   \
  V(JS_MODULE_TYPE)                                             \
  V(JS_GLOBAL_OBJECT_TYPE)                                      \
  V(JS_BUILTINS_OBJECT_TYPE)                                    \
  V(JS_GLOBAL_PROXY_TYPE)                                       \
  V(JS_ARRAY_TYPE)                                              \
  V(JS_ARRAY_BUFFER_TYPE)                                       \
  V(JS_TYPED_ARRAY_TYPE)                                        \
  V(JS_DATA_VIEW_TYPE)                                          \
  V(JS_PROXY_TYPE)                                              \
  V(JS_SET_TYPE)                                                \
  V(JS_MAP_TYPE)                                                \
  V(JS_SET_ITERATOR_TYPE)                                       \
  V(JS_MAP_ITERATOR_TYPE)                                       \
  V(JS_ITERATOR_RESULT_TYPE)                                    \
  V(JS_WEAK_MAP_TYPE)                                           \
  V(JS_WEAK_SET_TYPE)                                           \
  V(JS_REGEXP_TYPE)                                             \
                                                                \
  V(JS_FUNCTION_TYPE)                                           \
  V(JS_FUNCTION_PROXY_TYPE)                                     \
  V(DEBUG_INFO_TYPE)                                            \
  V(BREAK_POINT_INFO_TYPE)


// Since string types are not consecutive, this macro is used to
// iterate over them.
#define STRING_TYPE_LIST(V)                                                   \
  V(STRING_TYPE, kVariableSizeSentinel, string, String)                       \
  V(ONE_BYTE_STRING_TYPE, kVariableSizeSentinel, one_byte_string,             \
    OneByteString)                                                            \
  V(CONS_STRING_TYPE, ConsString::kSize, cons_string, ConsString)             \
  V(CONS_ONE_BYTE_STRING_TYPE, ConsString::kSize, cons_one_byte_string,       \
    ConsOneByteString)                                                        \
  V(SLICED_STRING_TYPE, SlicedString::kSize, sliced_string, SlicedString)     \
  V(SLICED_ONE_BYTE_STRING_TYPE, SlicedString::kSize, sliced_one_byte_string, \
    SlicedOneByteString)                                                      \
  V(EXTERNAL_STRING_TYPE, ExternalTwoByteString::kSize, external_string,      \
    ExternalString)                                                           \
  V(EXTERNAL_ONE_BYTE_STRING_TYPE, ExternalOneByteString::kSize,              \
    external_one_byte_string, ExternalOneByteString)                          \
  V(EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE, ExternalTwoByteString::kSize,    \
    external_string_with_one_byte_data, ExternalStringWithOneByteData)        \
  V(SHORT_EXTERNAL_STRING_TYPE, ExternalTwoByteString::kShortSize,            \
    short_external_string, ShortExternalString)                               \
  V(SHORT_EXTERNAL_ONE_BYTE_STRING_TYPE, ExternalOneByteString::kShortSize,   \
    short_external_one_byte_string, ShortExternalOneByteString)               \
  V(SHORT_EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE,                            \
    ExternalTwoByteString::kShortSize,                                        \
    short_external_string_with_one_byte_data,                                 \
    ShortExternalStringWithOneByteData)                                       \
                                                                              \
  V(INTERNALIZED_STRING_TYPE, kVariableSizeSentinel, internalized_string,     \
    InternalizedString)                                                       \
  V(ONE_BYTE_INTERNALIZED_STRING_TYPE, kVariableSizeSentinel,                 \
    one_byte_internalized_string, OneByteInternalizedString)                  \
  V(EXTERNAL_INTERNALIZED_STRING_TYPE, ExternalTwoByteString::kSize,          \
    external_internalized_string, ExternalInternalizedString)                 \
  V(EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE, ExternalOneByteString::kSize, \
    external_one_byte_internalized_string, ExternalOneByteInternalizedString) \
  V(EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE,                     \
    ExternalTwoByteString::kSize,                                             \
    external_internalized_string_with_one_byte_data,                          \
    ExternalInternalizedStringWithOneByteData)                                \
  V(SHORT_EXTERNAL_INTERNALIZED_STRING_TYPE,                                  \
    ExternalTwoByteString::kShortSize, short_external_internalized_string,    \
    ShortExternalInternalizedString)                                          \
  V(SHORT_EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE,                         \
    ExternalOneByteString::kShortSize,                                        \
    short_external_one_byte_internalized_string,                              \
    ShortExternalOneByteInternalizedString)                                   \
  V(SHORT_EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE,               \
    ExternalTwoByteString::kShortSize,                                        \
    short_external_internalized_string_with_one_byte_data,                    \
    ShortExternalInternalizedStringWithOneByteData)

// A struct is a simple object a set of object-valued fields.  Including an
// object type in this causes the compiler to generate most of the boilerplate
// code for the class including allocation and garbage collection routines,
// casts and predicates.  All you need to define is the class, methods and
// object verification routines.  Easy, no?
//
// Note that for subtle reasons related to the ordering or numerical values of
// type tags, elements in this list have to be added to the INSTANCE_TYPE_LIST
// manually.
#define STRUCT_LIST(V)                                                       \
  V(BOX, Box, box)                                                           \
  V(EXECUTABLE_ACCESSOR_INFO, ExecutableAccessorInfo,                        \
    executable_accessor_info)                                                \
  V(ACCESSOR_PAIR, AccessorPair, accessor_pair)                              \
  V(ACCESS_CHECK_INFO, AccessCheckInfo, access_check_info)                   \
  V(INTERCEPTOR_INFO, InterceptorInfo, interceptor_info)                     \
  V(CALL_HANDLER_INFO, CallHandlerInfo, call_handler_info)                   \
  V(FUNCTION_TEMPLATE_INFO, FunctionTemplateInfo, function_template_info)    \
  V(OBJECT_TEMPLATE_INFO, ObjectTemplateInfo, object_template_info)          \
  V(TYPE_SWITCH_INFO, TypeSwitchInfo, type_switch_info)                      \
  V(SCRIPT, Script, script)                                                  \
  V(ALLOCATION_SITE, AllocationSite, allocation_site)                        \
  V(ALLOCATION_MEMENTO, AllocationMemento, allocation_memento)               \
  V(CODE_CACHE, CodeCache, code_cache)                                       \
  V(POLYMORPHIC_CODE_CACHE, PolymorphicCodeCache, polymorphic_code_cache)    \
  V(TYPE_FEEDBACK_INFO, TypeFeedbackInfo, type_feedback_info)                \
  V(ALIASED_ARGUMENTS_ENTRY, AliasedArgumentsEntry, aliased_arguments_entry) \
  V(DEBUG_INFO, DebugInfo, debug_info)                                       \
  V(BREAK_POINT_INFO, BreakPointInfo, break_point_info)                      \
  V(PROTOTYPE_INFO, PrototypeInfo, prototype_info)                           \
  V(SLOPPY_BLOCK_WITH_EVAL_CONTEXT_EXTENSION,                                \
    SloppyBlockWithEvalContextExtension,                                     \
    sloppy_block_with_eval_context_extension)

// We use the full 8 bits of the instance_type field to encode heap object
// instance types.  The high-order bit (bit 7) is set if the object is not a
// string, and cleared if it is a string.
const uint32_t kIsNotStringMask = 0x80;
const uint32_t kStringTag = 0x0;
const uint32_t kNotStringTag = 0x80;

// Bit 6 indicates that the object is an internalized string (if set) or not.
// Bit 7 has to be clear as well.
const uint32_t kIsNotInternalizedMask = 0x40;
const uint32_t kNotInternalizedTag = 0x40;
const uint32_t kInternalizedTag = 0x0;

// If bit 7 is clear then bit 2 indicates whether the string consists of
// two-byte characters or one-byte characters.
const uint32_t kStringEncodingMask = 0x4;
const uint32_t kTwoByteStringTag = 0x0;
const uint32_t kOneByteStringTag = 0x4;

// If bit 7 is clear, the low-order 2 bits indicate the representation
// of the string.
const uint32_t kStringRepresentationMask = 0x03;
enum StringRepresentationTag {
  kSeqStringTag = 0x0,
  kConsStringTag = 0x1,
  kExternalStringTag = 0x2,
  kSlicedStringTag = 0x3
};
const uint32_t kIsIndirectStringMask = 0x1;
const uint32_t kIsIndirectStringTag = 0x1;
STATIC_ASSERT((kSeqStringTag & kIsIndirectStringMask) == 0);  // NOLINT
STATIC_ASSERT((kExternalStringTag & kIsIndirectStringMask) == 0);  // NOLINT
STATIC_ASSERT((kConsStringTag &
               kIsIndirectStringMask) == kIsIndirectStringTag);  // NOLINT
STATIC_ASSERT((kSlicedStringTag &
               kIsIndirectStringMask) == kIsIndirectStringTag);  // NOLINT

// Use this mask to distinguish between cons and slice only after making
// sure that the string is one of the two (an indirect string).
const uint32_t kSlicedNotConsMask = kSlicedStringTag & ~kConsStringTag;
STATIC_ASSERT(IS_POWER_OF_TWO(kSlicedNotConsMask));

// If bit 7 is clear, then bit 3 indicates whether this two-byte
// string actually contains one byte data.
const uint32_t kOneByteDataHintMask = 0x08;
const uint32_t kOneByteDataHintTag = 0x08;

// If bit 7 is clear and string representation indicates an external string,
// then bit 4 indicates whether the data pointer is cached.
const uint32_t kShortExternalStringMask = 0x10;
const uint32_t kShortExternalStringTag = 0x10;


// A ConsString with an empty string as the right side is a candidate
// for being shortcut by the garbage collector. We don't allocate any
// non-flat internalized strings, so we do not shortcut them thereby
// avoiding turning internalized strings into strings. The bit-masks
// below contain the internalized bit as additional safety.
// See heap.cc, mark-compact.cc and objects-visiting.cc.
const uint32_t kShortcutTypeMask =
    kIsNotStringMask |
    kIsNotInternalizedMask |
    kStringRepresentationMask;
const uint32_t kShortcutTypeTag = kConsStringTag | kNotInternalizedTag;

static inline bool IsShortcutCandidate(int type) {
  return ((type & kShortcutTypeMask) == kShortcutTypeTag);
}


enum InstanceType {
  // String types.
  INTERNALIZED_STRING_TYPE = kTwoByteStringTag | kSeqStringTag |
                             kInternalizedTag,  // FIRST_PRIMITIVE_TYPE
  ONE_BYTE_INTERNALIZED_STRING_TYPE =
      kOneByteStringTag | kSeqStringTag | kInternalizedTag,
  EXTERNAL_INTERNALIZED_STRING_TYPE =
      kTwoByteStringTag | kExternalStringTag | kInternalizedTag,
  EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE =
      kOneByteStringTag | kExternalStringTag | kInternalizedTag,
  EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE =
      EXTERNAL_INTERNALIZED_STRING_TYPE | kOneByteDataHintTag |
      kInternalizedTag,
  SHORT_EXTERNAL_INTERNALIZED_STRING_TYPE = EXTERNAL_INTERNALIZED_STRING_TYPE |
                                            kShortExternalStringTag |
                                            kInternalizedTag,
  SHORT_EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE =
      EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE | kShortExternalStringTag |
      kInternalizedTag,
  SHORT_EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE =
      EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE |
      kShortExternalStringTag | kInternalizedTag,
  STRING_TYPE = INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  ONE_BYTE_STRING_TYPE =
      ONE_BYTE_INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  CONS_STRING_TYPE = kTwoByteStringTag | kConsStringTag | kNotInternalizedTag,
  CONS_ONE_BYTE_STRING_TYPE =
      kOneByteStringTag | kConsStringTag | kNotInternalizedTag,
  SLICED_STRING_TYPE =
      kTwoByteStringTag | kSlicedStringTag | kNotInternalizedTag,
  SLICED_ONE_BYTE_STRING_TYPE =
      kOneByteStringTag | kSlicedStringTag | kNotInternalizedTag,
  EXTERNAL_STRING_TYPE =
      EXTERNAL_INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  EXTERNAL_ONE_BYTE_STRING_TYPE =
      EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE =
      EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE |
      kNotInternalizedTag,
  SHORT_EXTERNAL_STRING_TYPE =
      SHORT_EXTERNAL_INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  SHORT_EXTERNAL_ONE_BYTE_STRING_TYPE =
      SHORT_EXTERNAL_ONE_BYTE_INTERNALIZED_STRING_TYPE | kNotInternalizedTag,
  SHORT_EXTERNAL_STRING_WITH_ONE_BYTE_DATA_TYPE =
      SHORT_EXTERNAL_INTERNALIZED_STRING_WITH_ONE_BYTE_DATA_TYPE |
      kNotInternalizedTag,

  // Non-string names
  SYMBOL_TYPE = kNotStringTag,  // FIRST_NONSTRING_TYPE, LAST_NAME_TYPE

  // Other primitives (cannot contain non-map-word pointers to heap objects).
  HEAP_NUMBER_TYPE,
  SIMD128_VALUE_TYPE,
  ODDBALL_TYPE,  // LAST_PRIMITIVE_TYPE

  // Objects allocated in their own spaces (never in new space).
  MAP_TYPE,
  CODE_TYPE,

  // "Data", objects that cannot contain non-map-word pointers to heap
  // objects.
  MUTABLE_HEAP_NUMBER_TYPE,
  FOREIGN_TYPE,
  BYTE_ARRAY_TYPE,
  BYTECODE_ARRAY_TYPE,
  FREE_SPACE_TYPE,
  FIXED_INT8_ARRAY_TYPE,  // FIRST_FIXED_TYPED_ARRAY_TYPE
  FIXED_UINT8_ARRAY_TYPE,
  FIXED_INT16_ARRAY_TYPE,
  FIXED_UINT16_ARRAY_TYPE,
  FIXED_INT32_ARRAY_TYPE,
  FIXED_UINT32_ARRAY_TYPE,
  FIXED_FLOAT32_ARRAY_TYPE,
  FIXED_FLOAT64_ARRAY_TYPE,
  FIXED_UINT8_CLAMPED_ARRAY_TYPE,  // LAST_FIXED_TYPED_ARRAY_TYPE
  FIXED_DOUBLE_ARRAY_TYPE,
  FILLER_TYPE,  // LAST_DATA_TYPE

  // Structs.
  DECLARED_ACCESSOR_DESCRIPTOR_TYPE,
  DECLARED_ACCESSOR_INFO_TYPE,
  EXECUTABLE_ACCESSOR_INFO_TYPE,
  ACCESSOR_PAIR_TYPE,
  ACCESS_CHECK_INFO_TYPE,
  INTERCEPTOR_INFO_TYPE,
  CALL_HANDLER_INFO_TYPE,
  FUNCTION_TEMPLATE_INFO_TYPE,
  OBJECT_TEMPLATE_INFO_TYPE,
  SIGNATURE_INFO_TYPE,
  TYPE_SWITCH_INFO_TYPE,
  ALLOCATION_SITE_TYPE,
  ALLOCATION_MEMENTO_TYPE,
  SCRIPT_TYPE,
  CODE_CACHE_TYPE,
  POLYMORPHIC_CODE_CACHE_TYPE,
  TYPE_FEEDBACK_INFO_TYPE,
  ALIASED_ARGUMENTS_ENTRY_TYPE,
  BOX_TYPE,
  DEBUG_INFO_TYPE,
  BREAK_POINT_INFO_TYPE,
  FIXED_ARRAY_TYPE,
  SHARED_FUNCTION_INFO_TYPE,
  CELL_TYPE,
  WEAK_CELL_TYPE,
  PROPERTY_CELL_TYPE,
  PROTOTYPE_INFO_TYPE,
  SLOPPY_BLOCK_WITH_EVAL_CONTEXT_EXTENSION_TYPE,

  // All the following types are subtypes of JSReceiver, which corresponds to
  // objects in the JS sense. The first and the last type in this range are
  // the two forms of function. This organization enables using the same
  // compares for checking the JS_RECEIVER/SPEC_OBJECT range and the
  // NONCALLABLE_JS_OBJECT range.
  JS_FUNCTION_PROXY_TYPE,  // FIRST_JS_RECEIVER_TYPE, FIRST_JS_PROXY_TYPE
  JS_PROXY_TYPE,           // LAST_JS_PROXY_TYPE
  JS_VALUE_TYPE,           // FIRST_JS_OBJECT_TYPE
  JS_MESSAGE_OBJECT_TYPE,
  JS_DATE_TYPE,
  JS_OBJECT_TYPE,
  JS_CONTEXT_EXTENSION_OBJECT_TYPE,
  JS_GENERATOR_OBJECT_TYPE,
  JS_MODULE_TYPE,
  JS_GLOBAL_OBJECT_TYPE,
  JS_BUILTINS_OBJECT_TYPE,
  JS_GLOBAL_PROXY_TYPE,
  JS_ARRAY_TYPE,
  JS_ARRAY_BUFFER_TYPE,
  JS_TYPED_ARRAY_TYPE,
  JS_DATA_VIEW_TYPE,
  JS_SET_TYPE,
  JS_MAP_TYPE,
  JS_SET_ITERATOR_TYPE,
  JS_MAP_ITERATOR_TYPE,
  JS_ITERATOR_RESULT_TYPE,
  JS_WEAK_MAP_TYPE,
  JS_WEAK_SET_TYPE,
  JS_REGEXP_TYPE,
  JS_FUNCTION_TYPE,  // LAST_JS_OBJECT_TYPE, LAST_JS_RECEIVER_TYPE

  // Pseudo-types
  FIRST_TYPE = 0x0,
  LAST_TYPE = JS_FUNCTION_TYPE,
  FIRST_NAME_TYPE = FIRST_TYPE,
  LAST_NAME_TYPE = SYMBOL_TYPE,
  FIRST_UNIQUE_NAME_TYPE = INTERNALIZED_STRING_TYPE,
  LAST_UNIQUE_NAME_TYPE = SYMBOL_TYPE,
  FIRST_NONSTRING_TYPE = SYMBOL_TYPE,
  FIRST_PRIMITIVE_TYPE = FIRST_NAME_TYPE,
  LAST_PRIMITIVE_TYPE = ODDBALL_TYPE,
  // Boundaries for testing for a fixed typed array.
  FIRST_FIXED_TYPED_ARRAY_TYPE = FIXED_INT8_ARRAY_TYPE,
  LAST_FIXED_TYPED_ARRAY_TYPE = FIXED_UINT8_CLAMPED_ARRAY_TYPE,
  // Boundary for promotion to old space.
  LAST_DATA_TYPE = FILLER_TYPE,
  // Boundary for objects represented as JSReceiver (i.e. JSObject or JSProxy).
  // Note that there is no range for JSObject or JSProxy, since their subtypes
  // are not continuous in this enum! The enum ranges instead reflect the
  // external class names, where proxies are treated as either ordinary objects,
  // or functions.
  FIRST_JS_RECEIVER_TYPE = JS_FUNCTION_PROXY_TYPE,
  LAST_JS_RECEIVER_TYPE = LAST_TYPE,
  // Boundaries for testing the types represented as JSObject
  FIRST_JS_OBJECT_TYPE = JS_VALUE_TYPE,
  LAST_JS_OBJECT_TYPE = LAST_TYPE,
  // Boundaries for testing the types represented as JSProxy
  FIRST_JS_PROXY_TYPE = JS_FUNCTION_PROXY_TYPE,
  LAST_JS_PROXY_TYPE = JS_PROXY_TYPE,
  // Boundaries for testing whether the type is a JavaScript object.
  FIRST_SPEC_OBJECT_TYPE = FIRST_JS_RECEIVER_TYPE,
  LAST_SPEC_OBJECT_TYPE = LAST_JS_RECEIVER_TYPE,
  // Boundaries for testing the types for which typeof is "object".
  FIRST_NONCALLABLE_SPEC_OBJECT_TYPE = JS_PROXY_TYPE,
  LAST_NONCALLABLE_SPEC_OBJECT_TYPE = JS_REGEXP_TYPE,
  // Note that the types for which typeof is "function" are not continuous.
  // Define this so that we can put assertions on discrete checks.
  NUM_OF_CALLABLE_SPEC_OBJECT_TYPES = 2
};

STATIC_ASSERT(JS_OBJECT_TYPE == Internals::kJSObjectType);
STATIC_ASSERT(FIRST_NONSTRING_TYPE == Internals::kFirstNonstringType);
STATIC_ASSERT(ODDBALL_TYPE == Internals::kOddballType);
STATIC_ASSERT(FOREIGN_TYPE == Internals::kForeignType);


#define FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(V) \
  V(FAST_ELEMENTS_SUB_TYPE)                   \
  V(DICTIONARY_ELEMENTS_SUB_TYPE)             \
  V(FAST_PROPERTIES_SUB_TYPE)                 \
  V(DICTIONARY_PROPERTIES_SUB_TYPE)           \
  V(MAP_CODE_CACHE_SUB_TYPE)                  \
  V(SCOPE_INFO_SUB_TYPE)                      \
  V(STRING_TABLE_SUB_TYPE)                    \
  V(DESCRIPTOR_ARRAY_SUB_TYPE)                \
  V(TRANSITION_ARRAY_SUB_TYPE)

enum FixedArraySubInstanceType {
#define DEFINE_FIXED_ARRAY_SUB_INSTANCE_TYPE(name) name,
  FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(DEFINE_FIXED_ARRAY_SUB_INSTANCE_TYPE)
#undef DEFINE_FIXED_ARRAY_SUB_INSTANCE_TYPE
  LAST_FIXED_ARRAY_SUB_TYPE = TRANSITION_ARRAY_SUB_TYPE
};


// TODO(bmeurer): Remove this in favor of the ComparisonResult below.
enum CompareResult {
  LESS      = -1,
  EQUAL     =  0,
  GREATER   =  1,

  NOT_EQUAL = GREATER
};


// Result of an abstract relational comparison of x and y, implemented according
// to ES6 section 7.2.11 Abstract Relational Comparison.
enum class ComparisonResult {
  kLessThan,     // x < y
  kEqual,        // x = y
  kGreaterThan,  // x > x
  kUndefined     // at least one of x or y was undefined or NaN
};


#define DECL_BOOLEAN_ACCESSORS(name)   \
  inline bool name() const;            \
  inline void set_##name(bool value);  \


#define DECL_ACCESSORS(name, type)                                      \
  inline type* name() const;                                            \
  inline void set_##name(type* value,                                   \
                         WriteBarrierMode mode = UPDATE_WRITE_BARRIER); \


#define DECLARE_CAST(type)                              \
  INLINE(static type* cast(Object* object));            \
  INLINE(static const type* cast(const Object* object));


class AccessorPair;
class AllocationSite;
class AllocationSiteCreationContext;
class AllocationSiteUsageContext;
class Cell;
class ConsString;
class ElementsAccessor;
class FixedArrayBase;
class FunctionLiteral;
class GlobalObject;
class JSBuiltinsObject;
class LayoutDescriptor;
class LookupIterator;
class ObjectHashTable;
class ObjectVisitor;
class PropertyCell;
class SafepointEntry;
class SharedFunctionInfo;
class StringStream;
class TypeFeedbackInfo;
class TypeFeedbackVector;
class WeakCell;

// We cannot just say "class HeapType;" if it is created from a template... =8-?
template<class> class TypeImpl;
struct HeapTypeConfig;
typedef TypeImpl<HeapTypeConfig> HeapType;


// A template-ized version of the IsXXX functions.
template <class C> inline bool Is(Object* obj);

#ifdef VERIFY_HEAP
#define DECLARE_VERIFIER(Name) void Name##Verify();
#else
#define DECLARE_VERIFIER(Name)
#endif

#ifdef OBJECT_PRINT
#define DECLARE_PRINTER(Name) void Name##Print(std::ostream& os);  // NOLINT
#else
#define DECLARE_PRINTER(Name)
#endif


#define OBJECT_TYPE_LIST(V) \
  V(Smi)                    \
  V(HeapObject)             \
  V(Number)

#define HEAP_OBJECT_TYPE_LIST(V)   \
  V(HeapNumber)                    \
  V(MutableHeapNumber)             \
  V(Simd128Value)                  \
  V(Float32x4)                     \
  V(Int32x4)                       \
  V(Uint32x4)                      \
  V(Bool32x4)                      \
  V(Int16x8)                       \
  V(Uint16x8)                      \
  V(Bool16x8)                      \
  V(Int8x16)                       \
  V(Uint8x16)                      \
  V(Bool8x16)                      \
  V(Name)                          \
  V(UniqueName)                    \
  V(String)                        \
  V(SeqString)                     \
  V(ExternalString)                \
  V(ConsString)                    \
  V(SlicedString)                  \
  V(ExternalTwoByteString)         \
  V(ExternalOneByteString)         \
  V(SeqTwoByteString)              \
  V(SeqOneByteString)              \
  V(InternalizedString)            \
  V(Symbol)                        \
                                   \
  V(FixedTypedArrayBase)           \
  V(FixedUint8Array)               \
  V(FixedInt8Array)                \
  V(FixedUint16Array)              \
  V(FixedInt16Array)               \
  V(FixedUint32Array)              \
  V(FixedInt32Array)               \
  V(FixedFloat32Array)             \
  V(FixedFloat64Array)             \
  V(FixedUint8ClampedArray)        \
  V(ByteArray)                     \
  V(BytecodeArray)                 \
  V(FreeSpace)                     \
  V(JSReceiver)                    \
  V(JSObject)                      \
  V(JSContextExtensionObject)      \
  V(JSGeneratorObject)             \
  V(JSModule)                      \
  V(LayoutDescriptor)              \
  V(Map)                           \
  V(DescriptorArray)               \
  V(TransitionArray)               \
  V(TypeFeedbackVector)            \
  V(DeoptimizationInputData)       \
  V(DeoptimizationOutputData)      \
  V(DependentCode)                 \
  V(HandlerTable)                  \
  V(FixedArray)                    \
  V(FixedDoubleArray)              \
  V(WeakFixedArray)                \
  V(ArrayList)                     \
  V(Context)                       \
  V(ScriptContextTable)            \
  V(NativeContext)                 \
  V(ScopeInfo)                     \
  V(JSFunction)                    \
  V(Code)                          \
  V(Oddball)                       \
  V(SharedFunctionInfo)            \
  V(JSValue)                       \
  V(JSDate)                        \
  V(JSMessageObject)               \
  V(StringWrapper)                 \
  V(Foreign)                       \
  V(Boolean)                       \
  V(JSArray)                       \
  V(JSArrayBuffer)                 \
  V(JSArrayBufferView)             \
  V(JSTypedArray)                  \
  V(JSDataView)                    \
  V(JSProxy)                       \
  V(JSFunctionProxy)               \
  V(JSSet)                         \
  V(JSMap)                         \
  V(JSSetIterator)                 \
  V(JSMapIterator)                 \
  V(JSIteratorResult)              \
  V(JSWeakCollection)              \
  V(JSWeakMap)                     \
  V(JSWeakSet)                     \
  V(JSRegExp)                      \
  V(HashTable)                     \
  V(Dictionary)                    \
  V(StringTable)                   \
  V(NormalizedMapCache)            \
  V(CompilationCacheTable)         \
  V(CodeCacheHashTable)            \
  V(PolymorphicCodeCacheHashTable) \
  V(MapCache)                      \
  V(Primitive)                     \
  V(GlobalObject)                  \
  V(JSGlobalObject)                \
  V(JSBuiltinsObject)              \
  V(JSGlobalProxy)                 \
  V(UndetectableObject)            \
  V(AccessCheckNeeded)             \
  V(Cell)                          \
  V(PropertyCell)                  \
  V(WeakCell)                      \
  V(ObjectHashTable)               \
  V(WeakHashTable)                 \
  V(OrderedHashTable)

// Object is the abstract superclass for all classes in the
// object hierarchy.
// Object does not use any virtual functions to avoid the
// allocation of the C++ vtable.
// Since both Smi and HeapObject are subclasses of Object no
// data members can be present in Object.
class Object {
 public:
  // Type testing.
  bool IsObject() const { return true; }

#define IS_TYPE_FUNCTION_DECL(type_)  INLINE(bool Is##type_() const);
  OBJECT_TYPE_LIST(IS_TYPE_FUNCTION_DECL)
  HEAP_OBJECT_TYPE_LIST(IS_TYPE_FUNCTION_DECL)
#undef IS_TYPE_FUNCTION_DECL

  // A non-keyed store is of the form a.x = foo or a["x"] = foo whereas
  // a keyed store is of the form a[expression] = foo.
  enum StoreFromKeyed {
    MAY_BE_STORE_FROM_KEYED,
    CERTAINLY_NOT_STORE_FROM_KEYED
  };

  INLINE(bool IsFixedArrayBase() const);
  INLINE(bool IsExternal() const);
  INLINE(bool IsAccessorInfo() const);

  INLINE(bool IsStruct() const);
#define DECLARE_STRUCT_PREDICATE(NAME, Name, name) \
  INLINE(bool Is##Name() const);
  STRUCT_LIST(DECLARE_STRUCT_PREDICATE)
#undef DECLARE_STRUCT_PREDICATE

  // ES6, section 7.2.3 IsCallable.
  INLINE(bool IsCallable() const);

  // ES6, section 7.2.4 IsConstructor.
  INLINE(bool IsConstructor() const);

  INLINE(bool IsSpecObject()) const;
  INLINE(bool IsTemplateInfo()) const;
  INLINE(bool IsNameDictionary() const);
  INLINE(bool IsGlobalDictionary() const);
  INLINE(bool IsSeededNumberDictionary() const);
  INLINE(bool IsUnseededNumberDictionary() const);
  INLINE(bool IsOrderedHashSet() const);
  INLINE(bool IsOrderedHashMap() const);
  static bool IsPromise(Handle<Object> object);

  // Oddball testing.
  INLINE(bool IsUndefined() const);
  INLINE(bool IsNull() const);
  INLINE(bool IsTheHole() const);
  INLINE(bool IsException() const);
  INLINE(bool IsUninitialized() const);
  INLINE(bool IsTrue() const);
  INLINE(bool IsFalse() const);
  INLINE(bool IsArgumentsMarker() const);

  // Filler objects (fillers and free space objects).
  INLINE(bool IsFiller() const);

  // Extract the number.
  inline double Number() const;
  INLINE(bool IsNaN() const);
  INLINE(bool IsMinusZero() const);
  bool ToInt32(int32_t* value);
  bool ToUint32(uint32_t* value);

  inline Representation OptimalRepresentation();

  inline ElementsKind OptimalElementsKind();

  inline bool FitsRepresentation(Representation representation);

  // Checks whether two valid primitive encodings of a property name resolve to
  // the same logical property. E.g., the smi 1, the string "1" and the double
  // 1 all refer to the same property, so this helper will return true.
  inline bool KeyEquals(Object* other);

  Handle<HeapType> OptimalType(Isolate* isolate, Representation representation);

  inline static Handle<Object> NewStorageFor(Isolate* isolate,
                                             Handle<Object> object,
                                             Representation representation);

  inline static Handle<Object> WrapForRead(Isolate* isolate,
                                           Handle<Object> object,
                                           Representation representation);

  // Returns true if the object is of the correct type to be used as a
  // implementation of a JSObject's elements.
  inline bool HasValidElements();

  inline bool HasSpecificClassOf(String* name);

  bool BooleanValue();                                      // ECMA-262 9.2.

  // ES6 section 7.2.11 Abstract Relational Comparison
  MUST_USE_RESULT static Maybe<ComparisonResult> Compare(
      Handle<Object> x, Handle<Object> y, Strength strength = Strength::WEAK);

  // ES6 section 7.2.12 Abstract Equality Comparison
  MUST_USE_RESULT static Maybe<bool> Equals(Handle<Object> x, Handle<Object> y);

  // ES6 section 7.2.13 Strict Equality Comparison
  bool StrictEquals(Object* that);

  // Convert to a JSObject if needed.
  // native_context is used when creating wrapper object.
  static inline MaybeHandle<JSReceiver> ToObject(Isolate* isolate,
                                                 Handle<Object> object);
  MUST_USE_RESULT static MaybeHandle<JSReceiver> ToObject(
      Isolate* isolate, Handle<Object> object, Handle<Context> context);

  // ES6 section 7.1.14 ToPropertyKey
  MUST_USE_RESULT static inline MaybeHandle<Name> ToName(Isolate* isolate,
                                                         Handle<Object> input);

  // ES6 section 7.1.1 ToPrimitive
  MUST_USE_RESULT static inline MaybeHandle<Object> ToPrimitive(
      Handle<Object> input, ToPrimitiveHint hint = ToPrimitiveHint::kDefault);

  // ES6 section 7.1.3 ToNumber
  MUST_USE_RESULT static MaybeHandle<Object> ToNumber(Handle<Object> input);

  // ES6 section 7.1.12 ToString
  MUST_USE_RESULT static MaybeHandle<String> ToString(Isolate* isolate,
                                                      Handle<Object> input);

  // ES6 section 7.3.9 GetMethod
  MUST_USE_RESULT static MaybeHandle<Object> GetMethod(
      Handle<JSReceiver> receiver, Handle<Name> name);

  // ES6 section 12.5.6 The typeof Operator
  static Handle<String> TypeOf(Isolate* isolate, Handle<Object> object);

  // ES6 section 12.6 Multiplicative Operators
  MUST_USE_RESULT static MaybeHandle<Object> Multiply(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> Divide(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> Modulus(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);

  // ES6 section 12.7 Additive Operators
  MUST_USE_RESULT static MaybeHandle<Object> Add(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> Subtract(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);

  // ES6 section 12.8 Bitwise Shift Operators
  MUST_USE_RESULT static MaybeHandle<Object> ShiftLeft(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> ShiftRight(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> ShiftRightLogical(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);

  // ES6 section 12.9 Relational Operators
  MUST_USE_RESULT static inline Maybe<bool> GreaterThan(
      Handle<Object> x, Handle<Object> y, Strength strength = Strength::WEAK);
  MUST_USE_RESULT static inline Maybe<bool> GreaterThanOrEqual(
      Handle<Object> x, Handle<Object> y, Strength strength = Strength::WEAK);
  MUST_USE_RESULT static inline Maybe<bool> LessThan(
      Handle<Object> x, Handle<Object> y, Strength strength = Strength::WEAK);
  MUST_USE_RESULT static inline Maybe<bool> LessThanOrEqual(
      Handle<Object> x, Handle<Object> y, Strength strength = Strength::WEAK);

  // ES6 section 12.11 Binary Bitwise Operators
  MUST_USE_RESULT static MaybeHandle<Object> BitwiseAnd(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> BitwiseOr(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);
  MUST_USE_RESULT static MaybeHandle<Object> BitwiseXor(
      Isolate* isolate, Handle<Object> lhs, Handle<Object> rhs,
      Strength strength = Strength::WEAK);

  MUST_USE_RESULT static MaybeHandle<Object> GetProperty(
      LookupIterator* it, LanguageMode language_mode = SLOPPY);

  // Implementation of [[Put]], ECMA-262 5th edition, section 8.12.5.
  MUST_USE_RESULT static MaybeHandle<Object> SetProperty(
      Handle<Object> object, Handle<Name> name, Handle<Object> value,
      LanguageMode language_mode,
      StoreFromKeyed store_mode = MAY_BE_STORE_FROM_KEYED);

  MUST_USE_RESULT static MaybeHandle<Object> SetProperty(
      LookupIterator* it, Handle<Object> value, LanguageMode language_mode,
      StoreFromKeyed store_mode);

  MUST_USE_RESULT static MaybeHandle<Object> SetSuperProperty(
      LookupIterator* it, Handle<Object> value, LanguageMode language_mode,
      StoreFromKeyed store_mode);

  MUST_USE_RESULT static MaybeHandle<Object> ReadAbsentProperty(
      LookupIterator* it, LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> ReadAbsentProperty(
      Isolate* isolate, Handle<Object> receiver, Handle<Object> name,
      LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> WriteToReadOnlyProperty(
      LookupIterator* it, Handle<Object> value, LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> WriteToReadOnlyProperty(
      Isolate* isolate, Handle<Object> receiver, Handle<Object> name,
      Handle<Object> value, LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> RedefineNonconfigurableProperty(
      Isolate* isolate, Handle<Object> name, Handle<Object> value,
      LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> SetDataProperty(
      LookupIterator* it, Handle<Object> value);
  MUST_USE_RESULT static MaybeHandle<Object> AddDataProperty(
      LookupIterator* it, Handle<Object> value, PropertyAttributes attributes,
      LanguageMode language_mode, StoreFromKeyed store_mode);
  MUST_USE_RESULT static inline MaybeHandle<Object> GetPropertyOrElement(
      Handle<Object> object, Handle<Name> name,
      LanguageMode language_mode = SLOPPY);
  MUST_USE_RESULT static inline MaybeHandle<Object> GetProperty(
      Isolate* isolate, Handle<Object> object, const char* key,
      LanguageMode language_mode = SLOPPY);
  MUST_USE_RESULT static inline MaybeHandle<Object> GetProperty(
      Handle<Object> object, Handle<Name> name,
      LanguageMode language_mode = SLOPPY);

  MUST_USE_RESULT static MaybeHandle<Object> GetPropertyWithAccessor(
      LookupIterator* it, LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyWithAccessor(
      LookupIterator* it, Handle<Object> value, LanguageMode language_mode);

  MUST_USE_RESULT static MaybeHandle<Object> GetPropertyWithDefinedGetter(
      Handle<Object> receiver,
      Handle<JSReceiver> getter);
  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyWithDefinedSetter(
      Handle<Object> receiver,
      Handle<JSReceiver> setter,
      Handle<Object> value);

  MUST_USE_RESULT static inline MaybeHandle<Object> GetElement(
      Isolate* isolate, Handle<Object> object, uint32_t index,
      LanguageMode language_mode = SLOPPY);

  MUST_USE_RESULT static inline MaybeHandle<Object> SetElement(
      Isolate* isolate, Handle<Object> object, uint32_t index,
      Handle<Object> value, LanguageMode language_mode);

  static inline Handle<Object> GetPrototypeSkipHiddenPrototypes(
      Isolate* isolate, Handle<Object> receiver);

  bool HasInPrototypeChain(Isolate* isolate, Object* object);

  // Returns the permanent hash code associated with this object. May return
  // undefined if not yet created.
  Object* GetHash();

  // Returns undefined for JSObjects, but returns the hash code for simple
  // objects.  This avoids a double lookup in the cases where we know we will
  // add the hash to the JSObject if it does not already exist.
  Object* GetSimpleHash();

  // Returns the permanent hash code associated with this object depending on
  // the actual object type. May create and store a hash code if needed and none
  // exists.
  static Handle<Smi> GetOrCreateHash(Isolate* isolate, Handle<Object> object);

  // Checks whether this object has the same value as the given one.  This
  // function is implemented according to ES5, section 9.12 and can be used
  // to implement the Harmony "egal" function.
  bool SameValue(Object* other);

  // Checks whether this object has the same value as the given one.
  // +0 and -0 are treated equal. Everything else is the same as SameValue.
  // This function is implemented according to ES6, section 7.2.4 and is used
  // by ES6 Map and Set.
  bool SameValueZero(Object* other);

  // Tries to convert an object to an array length. Returns true and sets the
  // output parameter if it succeeds.
  inline bool ToArrayLength(uint32_t* index);

  // Tries to convert an object to an array index. Returns true and sets the
  // output parameter if it succeeds. Equivalent to ToArrayLength, but does not
  // allow kMaxUInt32.
  inline bool ToArrayIndex(uint32_t* index);

  // Returns true if this is a JSValue containing a string and the index is
  // < the length of the string.  Used to implement [] on strings.
  inline bool IsStringObjectWithCharacterAt(uint32_t index);

  DECLARE_VERIFIER(Object)
#ifdef VERIFY_HEAP
  // Verify a pointer is a valid object pointer.
  static void VerifyPointer(Object* p);
#endif

  inline void VerifyApiCallResultType();

  // Prints this object without details.
  void ShortPrint(FILE* out = stdout);

  // Prints this object without details to a message accumulator.
  void ShortPrint(StringStream* accumulator);

  void ShortPrint(std::ostream& os);  // NOLINT

  DECLARE_CAST(Object)

  // Layout description.
  static const int kHeaderSize = 0;  // Object does not take up any space.

#ifdef OBJECT_PRINT
  // For our gdb macros, we should perhaps change these in the future.
  void Print();

  // Prints this object with details.
  void Print(std::ostream& os);  // NOLINT
#else
  void Print() { ShortPrint(); }
  void Print(std::ostream& os) { ShortPrint(os); }  // NOLINT
#endif

 private:
  friend class LookupIterator;
  friend class PrototypeIterator;

  // Return the map of the root of object's prototype chain.
  Map* GetRootMap(Isolate* isolate);

  // Helper for SetProperty and SetSuperProperty.
  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyInternal(
      LookupIterator* it, Handle<Object> value, LanguageMode language_mode,
      StoreFromKeyed store_mode, bool* found);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};


// In objects.h to be usable without objects-inl.h inclusion.
bool Object::IsSmi() const { return HAS_SMI_TAG(this); }
bool Object::IsHeapObject() const { return Internals::HasHeapObjectTag(this); }


struct Brief {
  explicit Brief(const Object* const v) : value(v) {}
  const Object* value;
};


std::ostream& operator<<(std::ostream& os, const Brief& v);


// Smi represents integer Numbers that can be stored in 31 bits.
// Smis are immediate which means they are NOT allocated in the heap.
// The this pointer has the following format: [31 bit signed int] 0
// For long smis it has the following format:
//     [32 bit signed int] [31 bits zero padding] 0
// Smi stands for small integer.
class Smi: public Object {
 public:
  // Returns the integer value.
  inline int value() const { return Internals::SmiValue(this); }

  // Convert a value to a Smi object.
  static inline Smi* FromInt(int value) {
    DCHECK(Smi::IsValid(value));
    return reinterpret_cast<Smi*>(Internals::IntToSmi(value));
  }

  static inline Smi* FromIntptr(intptr_t value) {
    DCHECK(Smi::IsValid(value));
    int smi_shift_bits = kSmiTagSize + kSmiShiftSize;
    return reinterpret_cast<Smi*>((value << smi_shift_bits) | kSmiTag);
  }

  // Returns whether value can be represented in a Smi.
  static inline bool IsValid(intptr_t value) {
    bool result = Internals::IsValidSmi(value);
    DCHECK_EQ(result, value >= kMinValue && value <= kMaxValue);
    return result;
  }

  DECLARE_CAST(Smi)

  // Dispatched behavior.
  void SmiPrint(std::ostream& os) const;  // NOLINT
  DECLARE_VERIFIER(Smi)

  static const int kMinValue =
      (static_cast<unsigned int>(-1)) << (kSmiValueSize - 1);
  static const int kMaxValue = -(kMinValue + 1);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Smi);
};


// Heap objects typically have a map pointer in their first word.  However,
// during GC other data (e.g. mark bits, forwarding addresses) is sometimes
// encoded in the first word.  The class MapWord is an abstraction of the
// value in a heap object's first word.
class MapWord BASE_EMBEDDED {
 public:
  // Normal state: the map word contains a map pointer.

  // Create a map word from a map pointer.
  static inline MapWord FromMap(const Map* map);

  // View this map word as a map pointer.
  inline Map* ToMap();


  // Scavenge collection: the map word of live objects in the from space
  // contains a forwarding address (a heap object pointer in the to space).

  // True if this map word is a forwarding address for a scavenge
  // collection.  Only valid during a scavenge collection (specifically,
  // when all map words are heap object pointers, i.e. not during a full GC).
  inline bool IsForwardingAddress();

  // Create a map word from a forwarding address.
  static inline MapWord FromForwardingAddress(HeapObject* object);

  // View this map word as a forwarding address.
  inline HeapObject* ToForwardingAddress();

  static inline MapWord FromRawValue(uintptr_t value) {
    return MapWord(value);
  }

  inline uintptr_t ToRawValue() {
    return value_;
  }

 private:
  // HeapObject calls the private constructor and directly reads the value.
  friend class HeapObject;

  explicit MapWord(uintptr_t value) : value_(value) {}

  uintptr_t value_;
};


// The content of an heap object (except for the map pointer). kTaggedValues
// objects can contain both heap pointers and Smis, kMixedValues can contain
// heap pointers, Smis, and raw values (e.g. doubles or strings), and kRawValues
// objects can contain raw values and Smis.
enum class HeapObjectContents { kTaggedValues, kMixedValues, kRawValues };


// HeapObject is the superclass for all classes describing heap allocated
// objects.
class HeapObject: public Object {
 public:
  // [map]: Contains a map which contains the object's reflective
  // information.
  inline Map* map() const;
  inline void set_map(Map* value);
  // The no-write-barrier version.  This is OK if the object is white and in
  // new space, or if the value is an immortal immutable object, like the maps
  // of primitive (non-JS) objects like strings, heap numbers etc.
  inline void set_map_no_write_barrier(Map* value);

  // Get the map using acquire load.
  inline Map* synchronized_map();
  inline MapWord synchronized_map_word() const;

  // Set the map using release store
  inline void synchronized_set_map(Map* value);
  inline void synchronized_set_map_no_write_barrier(Map* value);
  inline void synchronized_set_map_word(MapWord map_word);

  // During garbage collection, the map word of a heap object does not
  // necessarily contain a map pointer.
  inline MapWord map_word() const;
  inline void set_map_word(MapWord map_word);

  // The Heap the object was allocated in. Used also to access Isolate.
  inline Heap* GetHeap() const;

  // Convenience method to get current isolate.
  inline Isolate* GetIsolate() const;

  // Converts an address to a HeapObject pointer.
  static inline HeapObject* FromAddress(Address address) {
    DCHECK_TAG_ALIGNED(address);
    return reinterpret_cast<HeapObject*>(address + kHeapObjectTag);
  }

  // Returns the address of this HeapObject.
  inline Address address() {
    return reinterpret_cast<Address>(this) - kHeapObjectTag;
  }

  // Iterates over pointers contained in the object (including the Map)
  void Iterate(ObjectVisitor* v);

  // Iterates over all pointers contained in the object except the
  // first map pointer.  The object type is given in the first
  // parameter. This function does not access the map pointer in the
  // object, and so is safe to call while the map pointer is modified.
  void IterateBody(InstanceType type, int object_size, ObjectVisitor* v);

  // Returns the heap object's size in bytes
  inline int Size();

  // Indicates what type of values this heap object may contain.
  inline HeapObjectContents ContentType();

  // Given a heap object's map pointer, returns the heap size in bytes
  // Useful when the map pointer field is used for other purposes.
  // GC internal.
  inline int SizeFromMap(Map* map);

  // Returns the field at offset in obj, as a read/write Object* reference.
  // Does no checking, and is safe to use during GC, while maps are invalid.
  // Does not invoke write barrier, so should only be assigned to
  // during marking GC.
  static inline Object** RawField(HeapObject* obj, int offset);

  // Adds the |code| object related to |name| to the code cache of this map. If
  // this map is a dictionary map that is shared, the map copied and installed
  // onto the object.
  static void UpdateMapCodeCache(Handle<HeapObject> object,
                                 Handle<Name> name,
                                 Handle<Code> code);

  DECLARE_CAST(HeapObject)

  // Return the write barrier mode for this. Callers of this function
  // must be able to present a reference to an DisallowHeapAllocation
  // object as a sign that they are not going to use this function
  // from code that allocates and thus invalidates the returned write
  // barrier mode.
  inline WriteBarrierMode GetWriteBarrierMode(
      const DisallowHeapAllocation& promise);

  // Dispatched behavior.
  void HeapObjectShortPrint(std::ostream& os);  // NOLINT
#ifdef OBJECT_PRINT
  void PrintHeader(std::ostream& os, const char* id);  // NOLINT
#endif
  DECLARE_PRINTER(HeapObject)
  DECLARE_VERIFIER(HeapObject)
#ifdef VERIFY_HEAP
  inline void VerifyObjectField(int offset);
  inline void VerifySmiField(int offset);

  // Verify a pointer is a valid HeapObject pointer that points to object
  // areas in the heap.
  static void VerifyHeapPointer(Object* p);
#endif

  inline AllocationAlignment RequiredAlignment();

  // Layout description.
  // First field in a heap object is map.
  static const int kMapOffset = Object::kHeaderSize;
  static const int kHeaderSize = kMapOffset + kPointerSize;

  STATIC_ASSERT(kMapOffset == Internals::kHeapObjectMapOffset);

 protected:
  // helpers for calling an ObjectVisitor to iterate over pointers in the
  // half-open range [start, end) specified as integer offsets
  inline void IteratePointers(ObjectVisitor* v, int start, int end);
  // as above, for the single element at "offset"
  inline void IteratePointer(ObjectVisitor* v, int offset);
  // as above, for the next code link of a code object.
  inline void IterateNextCodeLink(ObjectVisitor* v, int offset);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapObject);
};


// This class describes a body of an object of a fixed size
// in which all pointer fields are located in the [start_offset, end_offset)
// interval.
template<int start_offset, int end_offset, int size>
class FixedBodyDescriptor {
 public:
  static const int kStartOffset = start_offset;
  static const int kEndOffset = end_offset;
  static const int kSize = size;

  static inline void IterateBody(HeapObject* obj, ObjectVisitor* v);

  template<typename StaticVisitor>
  static inline void IterateBody(HeapObject* obj) {
    StaticVisitor::VisitPointers(HeapObject::RawField(obj, start_offset),
                                 HeapObject::RawField(obj, end_offset));
  }
};


// This class describes a body of an object of a variable size
// in which all pointer fields are located in the [start_offset, object_size)
// interval.
template<int start_offset>
class FlexibleBodyDescriptor {
 public:
  static const int kStartOffset = start_offset;

  static inline void IterateBody(HeapObject* obj,
                                 int object_size,
                                 ObjectVisitor* v);

  template<typename StaticVisitor>
  static inline void IterateBody(HeapObject* obj, int object_size) {
    StaticVisitor::VisitPointers(HeapObject::RawField(obj, start_offset),
                                 HeapObject::RawField(obj, object_size));
  }
};


// The HeapNumber class describes heap allocated numbers that cannot be
// represented in a Smi (small integer)
class HeapNumber: public HeapObject {
 public:
  // [value]: number value.
  inline double value() const;
  inline void set_value(double value);

  DECLARE_CAST(HeapNumber)

  // Dispatched behavior.
  bool HeapNumberBooleanValue();

  void HeapNumberPrint(std::ostream& os);  // NOLINT
  DECLARE_VERIFIER(HeapNumber)

  inline int get_exponent();
  inline int get_sign();

  // Layout description.
  static const int kValueOffset = HeapObject::kHeaderSize;
  // IEEE doubles are two 32 bit words.  The first is just mantissa, the second
  // is a mixture of sign, exponent and mantissa. The offsets of two 32 bit
  // words within double numbers are endian dependent and they are set
  // accordingly.
#if defined(V8_TARGET_LITTLE_ENDIAN)
  static const int kMantissaOffset = kValueOffset;
  static const int kExponentOffset = kValueOffset + 4;
#elif defined(V8_TARGET_BIG_ENDIAN)
  static const int kMantissaOffset = kValueOffset + 4;
  static const int kExponentOffset = kValueOffset;
#else
#error Unknown byte ordering
#endif

  static const int kSize = kValueOffset + kDoubleSize;
  static const uint32_t kSignMask = 0x80000000u;
  static const uint32_t kExponentMask = 0x7ff00000u;
  static const uint32_t kMantissaMask = 0xfffffu;
  static const int kMantissaBits = 52;
  static const int kExponentBits = 11;
  static const int kExponentBias = 1023;
  static const int kExponentShift = 20;
  static const int kInfinityOrNanExponent =
      (kExponentMask >> kExponentShift) - kExponentBias;
  static const int kMantissaBitsInTopWord = 20;
  static const int kNonMantissaBitsInTopWord = 12;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapNumber);
};


// The Simd128Value class describes heap allocated 128 bit SIMD values.
class Simd128Value : public HeapObject {
 public:
  DECLARE_CAST(Simd128Value)

  DECLARE_PRINTER(Simd128Value)
  DECLARE_VERIFIER(Simd128Value)

  static Handle<String> ToString(Handle<Simd128Value> input);

  // Equality operations.
  inline bool Equals(Simd128Value* that);
  static inline bool Equals(Handle<Simd128Value> one, Handle<Simd128Value> two);

  // Checks that another instance is bit-wise equal.
  bool BitwiseEquals(const Simd128Value* other) const;
  // Computes a hash from the 128 bit value, viewed as 4 32-bit integers.
  uint32_t Hash() const;
  // Copies the 16 bytes of SIMD data to the destination address.
  void CopyBits(void* destination) const;

  // Layout description.
  static const int kValueOffset = HeapObject::kHeaderSize;
  static const int kSize = kValueOffset + kSimd128Size;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Simd128Value);
};


// V has parameters (TYPE, Type, type, lane count, lane type)
#define SIMD128_TYPES(V)                       \
  V(FLOAT32X4, Float32x4, float32x4, 4, float) \
  V(INT32X4, Int32x4, int32x4, 4, int32_t)     \
  V(UINT32X4, Uint32x4, uint32x4, 4, uint32_t) \
  V(BOOL32X4, Bool32x4, bool32x4, 4, bool)     \
  V(INT16X8, Int16x8, int16x8, 8, int16_t)     \
  V(UINT16X8, Uint16x8, uint16x8, 8, uint16_t) \
  V(BOOL16X8, Bool16x8, bool16x8, 8, bool)     \
  V(INT8X16, Int8x16, int8x16, 16, int8_t)     \
  V(UINT8X16, Uint8x16, uint8x16, 16, uint8_t) \
  V(BOOL8X16, Bool8x16, bool8x16, 16, bool)

#define SIMD128_VALUE_CLASS(TYPE, Type, type, lane_count, lane_type) \
  class Type final : public Simd128Value {                           \
   public:                                                           \
    inline lane_type get_lane(int lane) const;                       \
    inline void set_lane(int lane, lane_type value);                 \
                                                                     \
    DECLARE_CAST(Type)                                               \
                                                                     \
    DECLARE_PRINTER(Type)                                            \
                                                                     \
    static Handle<String> ToString(Handle<Type> input);              \
                                                                     \
    inline bool Equals(Type* that);                                  \
                                                                     \
   private:                                                          \
    DISALLOW_IMPLICIT_CONSTRUCTORS(Type);                            \
  };
SIMD128_TYPES(SIMD128_VALUE_CLASS)
#undef SIMD128_VALUE_CLASS


enum EnsureElementsMode {
  DONT_ALLOW_DOUBLE_ELEMENTS,
  ALLOW_COPIED_DOUBLE_ELEMENTS,
  ALLOW_CONVERTED_DOUBLE_ELEMENTS
};


// Indicator for one component of an AccessorPair.
enum AccessorComponent {
  ACCESSOR_GETTER,
  ACCESSOR_SETTER
};


// JSReceiver includes types on which properties can be defined, i.e.,
// JSObject and JSProxy.
class JSReceiver: public HeapObject {
 public:
  DECLARE_CAST(JSReceiver)

  // ES6 section 7.1.1 ToPrimitive
  MUST_USE_RESULT static MaybeHandle<Object> ToPrimitive(
      Handle<JSReceiver> receiver,
      ToPrimitiveHint hint = ToPrimitiveHint::kDefault);
  MUST_USE_RESULT static MaybeHandle<Object> OrdinaryToPrimitive(
      Handle<JSReceiver> receiver, OrdinaryToPrimitiveHint hint);

  // Implementation of [[HasProperty]], ECMA-262 5th edition, section 8.12.6.
  MUST_USE_RESULT static inline Maybe<bool> HasProperty(
      Handle<JSReceiver> object, Handle<Name> name);
  MUST_USE_RESULT static inline Maybe<bool> HasOwnProperty(Handle<JSReceiver>,
                                                           Handle<Name> name);
  MUST_USE_RESULT static inline Maybe<bool> HasElement(
      Handle<JSReceiver> object, uint32_t index);
  MUST_USE_RESULT static inline Maybe<bool> HasOwnElement(
      Handle<JSReceiver> object, uint32_t index);

  // Implementation of [[Delete]], ECMA-262 5th edition, section 8.12.7.
  MUST_USE_RESULT static MaybeHandle<Object> DeletePropertyOrElement(
      Handle<JSReceiver> object, Handle<Name> name,
      LanguageMode language_mode = SLOPPY);
  MUST_USE_RESULT static MaybeHandle<Object> DeleteProperty(
      Handle<JSReceiver> object, Handle<Name> name,
      LanguageMode language_mode = SLOPPY);
  MUST_USE_RESULT static MaybeHandle<Object> DeleteProperty(
      LookupIterator* it, LanguageMode language_mode);
  MUST_USE_RESULT static MaybeHandle<Object> DeleteElement(
      Handle<JSReceiver> object, uint32_t index,
      LanguageMode language_mode = SLOPPY);

  // Tests for the fast common case for property enumeration.
  bool IsSimpleEnum();

  // Returns the class name ([[Class]] property in the specification).
  String* class_name();

  // Returns the constructor name (the name (possibly, inferred name) of the
  // function that was used to instantiate the object).
  String* constructor_name();

  MUST_USE_RESULT static inline Maybe<PropertyAttributes> GetPropertyAttributes(
      Handle<JSReceiver> object, Handle<Name> name);
  MUST_USE_RESULT static inline Maybe<PropertyAttributes>
  GetOwnPropertyAttributes(Handle<JSReceiver> object, Handle<Name> name);

  MUST_USE_RESULT static inline Maybe<PropertyAttributes> GetElementAttributes(
      Handle<JSReceiver> object, uint32_t index);
  MUST_USE_RESULT static inline Maybe<PropertyAttributes>
  GetOwnElementAttributes(Handle<JSReceiver> object, uint32_t index);

  MUST_USE_RESULT static Maybe<PropertyAttributes> GetPropertyAttributes(
      LookupIterator* it);


  static Handle<Object> GetDataProperty(Handle<JSReceiver> object,
                                        Handle<Name> name);
  static Handle<Object> GetDataProperty(LookupIterator* it);


  // Retrieves a permanent object identity hash code. The undefined value might
  // be returned in case no hash was created yet.
  inline Object* GetIdentityHash();

  // Retrieves a permanent object identity hash code. May create and store a
  // hash code if needed and none exists.
  inline static Handle<Smi> GetOrCreateIdentityHash(
      Handle<JSReceiver> object);

  enum KeyCollectionType { OWN_ONLY, INCLUDE_PROTOS };

  // Computes the enumerable keys for a JSObject. Used for implementing
  // "for (n in object) { }".
  MUST_USE_RESULT static MaybeHandle<FixedArray> GetKeys(
      Handle<JSReceiver> object,
      KeyCollectionType type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSReceiver);
};


// The JSObject describes real heap allocated JavaScript objects with
// properties.
// Note that the map of JSObject changes during execution to enable inline
// caching.
class JSObject: public JSReceiver {
 public:
  // [properties]: Backing storage for properties.
  // properties is a FixedArray in the fast case and a Dictionary in the
  // slow case.
  DECL_ACCESSORS(properties, FixedArray)  // Get and set fast properties.
  inline void initialize_properties();
  inline bool HasFastProperties();
  // Gets slow properties for non-global objects.
  inline NameDictionary* property_dictionary();
  // Gets global object properties.
  inline GlobalDictionary* global_dictionary();

  // [elements]: The elements (properties with names that are integers).
  //
  // Elements can be in two general modes: fast and slow. Each mode
  // corrensponds to a set of object representations of elements that
  // have something in common.
  //
  // In the fast mode elements is a FixedArray and so each element can
  // be quickly accessed. This fact is used in the generated code. The
  // elements array can have one of three maps in this mode:
  // fixed_array_map, sloppy_arguments_elements_map or
  // fixed_cow_array_map (for copy-on-write arrays). In the latter case
  // the elements array may be shared by a few objects and so before
  // writing to any element the array must be copied. Use
  // EnsureWritableFastElements in this case.
  //
  // In the slow mode the elements is either a NumberDictionary, a
  // FixedArray parameter map for a (sloppy) arguments object.
  DECL_ACCESSORS(elements, FixedArrayBase)
  inline void initialize_elements();
  static void ResetElements(Handle<JSObject> object);
  static inline void SetMapAndElements(Handle<JSObject> object,
                                       Handle<Map> map,
                                       Handle<FixedArrayBase> elements);
  inline ElementsKind GetElementsKind();
  ElementsAccessor* GetElementsAccessor();
  // Returns true if an object has elements of FAST_SMI_ELEMENTS ElementsKind.
  inline bool HasFastSmiElements();
  // Returns true if an object has elements of FAST_ELEMENTS ElementsKind.
  inline bool HasFastObjectElements();
  // Returns true if an object has elements of FAST_ELEMENTS or
  // FAST_SMI_ONLY_ELEMENTS.
  inline bool HasFastSmiOrObjectElements();
  // Returns true if an object has any of the fast elements kinds.
  inline bool HasFastElements();
  // Returns true if an object has elements of FAST_DOUBLE_ELEMENTS
  // ElementsKind.
  inline bool HasFastDoubleElements();
  // Returns true if an object has elements of FAST_HOLEY_*_ELEMENTS
  // ElementsKind.
  inline bool HasFastHoleyElements();
  inline bool HasSloppyArgumentsElements();
  inline bool HasDictionaryElements();

  inline bool HasFixedTypedArrayElements();

  inline bool HasFixedUint8ClampedElements();
  inline bool HasFixedArrayElements();
  inline bool HasFixedInt8Elements();
  inline bool HasFixedUint8Elements();
  inline bool HasFixedInt16Elements();
  inline bool HasFixedUint16Elements();
  inline bool HasFixedInt32Elements();
  inline bool HasFixedUint32Elements();
  inline bool HasFixedFloat32Elements();
  inline bool HasFixedFloat64Elements();

  inline bool HasFastArgumentsElements();
  inline bool HasSlowArgumentsElements();
  inline SeededNumberDictionary* element_dictionary();  // Gets slow elements.

  // Requires: HasFastElements().
  static Handle<FixedArray> EnsureWritableFastElements(
      Handle<JSObject> object);

  // Collects elements starting at index 0.
  // Undefined values are placed after non-undefined values.
  // Returns the number of non-undefined values.
  static Handle<Object> PrepareElementsForSort(Handle<JSObject> object,
                                               uint32_t limit);
  // As PrepareElementsForSort, but only on objects where elements is
  // a dictionary, and it will stay a dictionary.  Collates undefined and
  // unexisting elements below limit from position zero of the elements.
  static Handle<Object> PrepareSlowElementsForSort(Handle<JSObject> object,
                                                   uint32_t limit);

  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyWithInterceptor(
      LookupIterator* it, Handle<Object> value);

  // SetLocalPropertyIgnoreAttributes converts callbacks to fields. We need to
  // grant an exemption to ExecutableAccessor callbacks in some cases.
  enum ExecutableAccessorInfoHandling { DEFAULT_HANDLING, DONT_FORCE_FIELD };

  MUST_USE_RESULT static MaybeHandle<Object> DefineOwnPropertyIgnoreAttributes(
      LookupIterator* it, Handle<Object> value, PropertyAttributes attributes,
      ExecutableAccessorInfoHandling handling = DEFAULT_HANDLING);

  MUST_USE_RESULT static MaybeHandle<Object> SetOwnPropertyIgnoreAttributes(
      Handle<JSObject> object, Handle<Name> name, Handle<Object> value,
      PropertyAttributes attributes,
      ExecutableAccessorInfoHandling handling = DEFAULT_HANDLING);

  MUST_USE_RESULT static MaybeHandle<Object> SetOwnElementIgnoreAttributes(
      Handle<JSObject> object, uint32_t index, Handle<Object> value,
      PropertyAttributes attributes,
      ExecutableAccessorInfoHandling handling = DEFAULT_HANDLING);

  // Equivalent to one of the above depending on whether |name| can be converted
  // to an array index.
  MUST_USE_RESULT static MaybeHandle<Object>
  DefinePropertyOrElementIgnoreAttributes(
      Handle<JSObject> object, Handle<Name> name, Handle<Object> value,
      PropertyAttributes attributes = NONE,
      ExecutableAccessorInfoHandling handling = DEFAULT_HANDLING);

  // Adds or reconfigures a property to attributes NONE. It will fail when it
  // cannot.
  MUST_USE_RESULT static Maybe<bool> CreateDataProperty(LookupIterator* it,
                                                        Handle<Object> value);

  static void AddProperty(Handle<JSObject> object, Handle<Name> name,
                          Handle<Object> value, PropertyAttributes attributes);

  MUST_USE_RESULT static MaybeHandle<Object> AddDataElement(
      Handle<JSObject> receiver, uint32_t index, Handle<Object> value,
      PropertyAttributes attributes);

  // Extend the receiver with a single fast property appeared first in the
  // passed map. This also extends the property backing store if necessary.
  static void AllocateStorageForMap(Handle<JSObject> object, Handle<Map> map);

  // Migrates the given object to a map whose field representations are the
  // lowest upper bound of all known representations for that field.
  static void MigrateInstance(Handle<JSObject> instance);

  // Migrates the given object only if the target map is already available,
  // or returns false if such a map is not yet available.
  static bool TryMigrateInstance(Handle<JSObject> instance);

  // Sets the property value in a normalized object given (key, value, details).
  // Handles the special representation of JS global objects.
  static void SetNormalizedProperty(Handle<JSObject> object, Handle<Name> name,
                                    Handle<Object> value,
                                    PropertyDetails details);
  static void SetDictionaryElement(Handle<JSObject> object, uint32_t index,
                                   Handle<Object> value,
                                   PropertyAttributes attributes);
  static void SetDictionaryArgumentsElement(Handle<JSObject> object,
                                            uint32_t index,
                                            Handle<Object> value,
                                            PropertyAttributes attributes);

  static void OptimizeAsPrototype(Handle<JSObject> object,
                                  PrototypeOptimizationMode mode);
  static void ReoptimizeIfPrototype(Handle<JSObject> object);
  static void LazyRegisterPrototypeUser(Handle<Map> user, Isolate* isolate);
  static bool UnregisterPrototypeUser(Handle<Map> user, Isolate* isolate);
  static void InvalidatePrototypeChains(Map* map);

  // Alternative implementation of WeakFixedArray::NullCallback.
  class PrototypeRegistryCompactionCallback {
   public:
    static void Callback(Object* value, int old_index, int new_index);
  };

  // Retrieve interceptors.
  InterceptorInfo* GetNamedInterceptor();
  InterceptorInfo* GetIndexedInterceptor();

  // Used from JSReceiver.
  MUST_USE_RESULT static Maybe<PropertyAttributes>
  GetPropertyAttributesWithInterceptor(LookupIterator* it);
  MUST_USE_RESULT static Maybe<PropertyAttributes>
      GetPropertyAttributesWithFailedAccessCheck(LookupIterator* it);

  // Retrieves an AccessorPair property from the given object. Might return
  // undefined if the property doesn't exist or is of a different kind.
  MUST_USE_RESULT static MaybeHandle<Object> GetAccessor(
      Handle<JSObject> object,
      Handle<Name> name,
      AccessorComponent component);

  // Defines an AccessorPair property on the given object.
  // TODO(mstarzinger): Rename to SetAccessor().
  static MaybeHandle<Object> DefineAccessor(Handle<JSObject> object,
                                            Handle<Name> name,
                                            Handle<Object> getter,
                                            Handle<Object> setter,
                                            PropertyAttributes attributes);

  // Defines an AccessorInfo property on the given object.
  MUST_USE_RESULT static MaybeHandle<Object> SetAccessor(
      Handle<JSObject> object,
      Handle<AccessorInfo> info);

  // The result must be checked first for exceptions. If there's no exception,
  // the output parameter |done| indicates whether the interceptor has a result
  // or not.
  MUST_USE_RESULT static MaybeHandle<Object> GetPropertyWithInterceptor(
      LookupIterator* it, bool* done);

  // Accessors for hidden properties object.
  //
  // Hidden properties are not own properties of the object itself.
  // Instead they are stored in an auxiliary structure kept as an own
  // property with a special name Heap::hidden_string(). But if the
  // receiver is a JSGlobalProxy then the auxiliary object is a property
  // of its prototype, and if it's a detached proxy, then you can't have
  // hidden properties.

  // Sets a hidden property on this object. Returns this object if successful,
  // undefined if called on a detached proxy.
  static Handle<Object> SetHiddenProperty(Handle<JSObject> object,
                                          Handle<Name> key,
                                          Handle<Object> value);
  // Gets the value of a hidden property with the given key. Returns the hole
  // if the property doesn't exist (or if called on a detached proxy),
  // otherwise returns the value set for the key.
  Object* GetHiddenProperty(Handle<Name> key);
  // Deletes a hidden property. Deleting a non-existing property is
  // considered successful.
  static void DeleteHiddenProperty(Handle<JSObject> object,
                                   Handle<Name> key);
  // Returns true if the object has a property with the hidden string as name.
  static bool HasHiddenProperties(Handle<JSObject> object);

  static void SetIdentityHash(Handle<JSObject> object, Handle<Smi> hash);

  static void ValidateElements(Handle<JSObject> object);

  // Makes sure that this object can contain HeapObject as elements.
  static inline void EnsureCanContainHeapObjectElements(Handle<JSObject> obj);

  // Makes sure that this object can contain the specified elements.
  static inline void EnsureCanContainElements(
      Handle<JSObject> object,
      Object** elements,
      uint32_t count,
      EnsureElementsMode mode);
  static inline void EnsureCanContainElements(
      Handle<JSObject> object,
      Handle<FixedArrayBase> elements,
      uint32_t length,
      EnsureElementsMode mode);
  static void EnsureCanContainElements(
      Handle<JSObject> object,
      Arguments* arguments,
      uint32_t first_arg,
      uint32_t arg_count,
      EnsureElementsMode mode);

  // Would we convert a fast elements array to dictionary mode given
  // an access at key?
  bool WouldConvertToSlowElements(uint32_t index);

  // Computes the new capacity when expanding the elements of a JSObject.
  static uint32_t NewElementsCapacity(uint32_t old_capacity) {
    // (old_capacity + 50%) + 16
    return old_capacity + (old_capacity >> 1) + 16;
  }

  // These methods do not perform access checks!
  static void UpdateAllocationSite(Handle<JSObject> object,
                                   ElementsKind to_kind);

  // Lookup interceptors are used for handling properties controlled by host
  // objects.
  inline bool HasNamedInterceptor();
  inline bool HasIndexedInterceptor();

  // Computes the enumerable keys from interceptors. Used for debug mirrors and
  // by JSReceiver::GetKeys.
  MUST_USE_RESULT static MaybeHandle<JSObject> GetKeysForNamedInterceptor(
      Handle<JSObject> object,
      Handle<JSReceiver> receiver);
  MUST_USE_RESULT static MaybeHandle<JSObject> GetKeysForIndexedInterceptor(
      Handle<JSObject> object,
      Handle<JSReceiver> receiver);

  // Support functions for v8 api (needed for correct interceptor behavior).
  MUST_USE_RESULT static Maybe<bool> HasRealNamedProperty(
      Handle<JSObject> object, Handle<Name> name);
  MUST_USE_RESULT static Maybe<bool> HasRealElementProperty(
      Handle<JSObject> object, uint32_t index);
  MUST_USE_RESULT static Maybe<bool> HasRealNamedCallbackProperty(
      Handle<JSObject> object, Handle<Name> name);

  // Get the header size for a JSObject.  Used to compute the index of
  // internal fields as well as the number of internal fields.
  inline int GetHeaderSize();

  inline int GetInternalFieldCount();
  inline int GetInternalFieldOffset(int index);
  inline Object* GetInternalField(int index);
  inline void SetInternalField(int index, Object* value);
  inline void SetInternalField(int index, Smi* value);

  // Returns the number of properties on this object filtering out properties
  // with the specified attributes (ignoring interceptors).
  int NumberOfOwnProperties(PropertyAttributes filter = NONE);
  // Fill in details for properties into storage starting at the specified
  // index. Returns the number of properties added.
  int GetOwnPropertyNames(FixedArray* storage, int index,
                          PropertyAttributes filter = NONE);

  // Returns the number of properties on this object filtering out properties
  // with the specified attributes (ignoring interceptors).
  int NumberOfOwnElements(PropertyAttributes filter);
  // Returns the number of enumerable elements (ignoring interceptors).
  int NumberOfEnumElements();
  // Returns the number of elements on this object filtering out elements
  // with the specified attributes (ignoring interceptors).
  int GetOwnElementKeys(FixedArray* storage, PropertyAttributes filter);
  // Count and fill in the enumerable elements into storage.
  // (storage->length() == NumberOfEnumElements()).
  // If storage is NULL, will count the elements without adding
  // them to any storage.
  // Returns the number of enumerable elements.
  int GetEnumElementKeys(FixedArray* storage);

  static Handle<FixedArray> GetEnumPropertyKeys(Handle<JSObject> object,
                                                bool cache_result);

  // Returns a new map with all transitions dropped from the object's current
  // map and the ElementsKind set.
  static Handle<Map> GetElementsTransitionMap(Handle<JSObject> object,
                                              ElementsKind to_kind);
  static void TransitionElementsKind(Handle<JSObject> object,
                                     ElementsKind to_kind);

  // Always use this to migrate an object to a new map.
  // |expected_additional_properties| is only used for fast-to-slow transitions
  // and ignored otherwise.
  static void MigrateToMap(Handle<JSObject> object, Handle<Map> new_map,
                           int expected_additional_properties = 0);

  // Convert the object to use the canonical dictionary
  // representation. If the object is expected to have additional properties
  // added this number can be indicated to have the backing store allocated to
  // an initial capacity for holding these properties.
  static void NormalizeProperties(Handle<JSObject> object,
                                  PropertyNormalizationMode mode,
                                  int expected_additional_properties,
                                  const char* reason);

  // Convert and update the elements backing store to be a
  // SeededNumberDictionary dictionary.  Returns the backing after conversion.
  static Handle<SeededNumberDictionary> NormalizeElements(
      Handle<JSObject> object);

  void RequireSlowElements(SeededNumberDictionary* dictionary);

  // Transform slow named properties to fast variants.
  static void MigrateSlowToFast(Handle<JSObject> object,
                                int unused_property_fields, const char* reason);

  inline bool IsUnboxedDoubleField(FieldIndex index);

  // Access fast-case object properties at index.
  static Handle<Object> FastPropertyAt(Handle<JSObject> object,
                                       Representation representation,
                                       FieldIndex index);
  inline Object* RawFastPropertyAt(FieldIndex index);
  inline double RawFastDoublePropertyAt(FieldIndex index);

  inline void FastPropertyAtPut(FieldIndex index, Object* value);
  inline void RawFastPropertyAtPut(FieldIndex index, Object* value);
  inline void RawFastDoublePropertyAtPut(FieldIndex index, double value);
  inline void WriteToField(int descriptor, Object* value);

  // Access to in object properties.
  inline int GetInObjectPropertyOffset(int index);
  inline Object* InObjectPropertyAt(int index);
  inline Object* InObjectPropertyAtPut(int index,
                                       Object* value,
                                       WriteBarrierMode mode
                                       = UPDATE_WRITE_BARRIER);

  // Set the object's prototype (only JSReceiver and null are allowed values).
  MUST_USE_RESULT static MaybeHandle<Object> SetPrototype(
      Handle<JSObject> object, Handle<Object> value, bool from_javascript);

  // Initializes the body after properties slot, properties slot is
  // initialized by set_properties.  Fill the pre-allocated fields with
  // pre_allocated_value and the rest with filler_value.
  // Note: this call does not update write barrier, the caller is responsible
  // to ensure that |filler_value| can be collected without WB here.
  inline void InitializeBody(Map* map,
                             Object* pre_allocated_value,
                             Object* filler_value);

  // Check whether this object references another object
  bool ReferencesObject(Object* obj);

  // Disalow further properties to be added to the oject.
  MUST_USE_RESULT static MaybeHandle<Object> PreventExtensions(
      Handle<JSObject> object);

  bool IsExtensible();

  // ES5 Object.seal
  MUST_USE_RESULT static MaybeHandle<Object> Seal(Handle<JSObject> object);

  // ES5 Object.freeze
  MUST_USE_RESULT static MaybeHandle<Object> Freeze(Handle<JSObject> object);

  // Called the first time an object is observed with ES7 Object.observe.
  static void SetObserved(Handle<JSObject> object);

  // Copy object.
  enum DeepCopyHints { kNoHints = 0, kObjectIsShallow = 1 };

  MUST_USE_RESULT static MaybeHandle<JSObject> DeepCopy(
      Handle<JSObject> object,
      AllocationSiteUsageContext* site_context,
      DeepCopyHints hints = kNoHints);
  MUST_USE_RESULT static MaybeHandle<JSObject> DeepWalk(
      Handle<JSObject> object,
      AllocationSiteCreationContext* site_context);

  DECLARE_CAST(JSObject)

  // Dispatched behavior.
  void JSObjectShortPrint(StringStream* accumulator);
  DECLARE_PRINTER(JSObject)
  DECLARE_VERIFIER(JSObject)
#ifdef OBJECT_PRINT
  void PrintProperties(std::ostream& os);   // NOLINT
  void PrintElements(std::ostream& os);     // NOLINT
#endif
#if defined(DEBUG) || defined(OBJECT_PRINT)
  void PrintTransitions(std::ostream& os);  // NOLINT
#endif

  static void PrintElementsTransition(
      FILE* file, Handle<JSObject> object,
      ElementsKind from_kind, Handle<FixedArrayBase> from_elements,
      ElementsKind to_kind, Handle<FixedArrayBase> to_elements);

  void PrintInstanceMigration(FILE* file, Map* original_map, Map* new_map);

#ifdef DEBUG
  // Structure for collecting spill information about JSObjects.
  class SpillInformation {
   public:
    void Clear();
    void Print();
    int number_of_objects_;
    int number_of_objects_with_fast_properties_;
    int number_of_objects_with_fast_elements_;
    int number_of_fast_used_fields_;
    int number_of_fast_unused_fields_;
    int number_of_slow_used_properties_;
    int number_of_slow_unused_properties_;
    int number_of_fast_used_elements_;
    int number_of_fast_unused_elements_;
    int number_of_slow_used_elements_;
    int number_of_slow_unused_elements_;
  };

  void IncrementSpillStatistics(SpillInformation* info);
#endif

#ifdef VERIFY_HEAP
  // If a GC was caused while constructing this object, the elements pointer
  // may point to a one pointer filler map. The object won't be rooted, but
  // our heap verification code could stumble across it.
  bool ElementsAreSafeToExamine();
#endif

  Object* SlowReverseLookup(Object* value);

  // Maximal number of elements (numbered 0 .. kMaxElementCount - 1).
  // Also maximal value of JSArray's length property.
  static const uint32_t kMaxElementCount = 0xffffffffu;

  // Constants for heuristics controlling conversion of fast elements
  // to slow elements.

  // Maximal gap that can be introduced by adding an element beyond
  // the current elements length.
  static const uint32_t kMaxGap = 1024;

  // Maximal length of fast elements array that won't be checked for
  // being dense enough on expansion.
  static const int kMaxUncheckedFastElementsLength = 5000;

  // Same as above but for old arrays. This limit is more strict. We
  // don't want to be wasteful with long lived objects.
  static const int kMaxUncheckedOldFastElementsLength = 500;

  // Note that Page::kMaxRegularHeapObjectSize puts a limit on
  // permissible values (see the DCHECK in heap.cc).
  static const int kInitialMaxFastElementArray = 100000;

  // This constant applies only to the initial map of "global.Object" and
  // not to arbitrary other JSObject maps.
  static const int kInitialGlobalObjectUnusedPropertiesCount = 4;

  static const int kMaxInstanceSize = 255 * kPointerSize;
  // When extending the backing storage for property values, we increase
  // its size by more than the 1 entry necessary, so sequentially adding fields
  // to the same object requires fewer allocations and copies.
  static const int kFieldsAdded = 3;

  // Layout description.
  static const int kPropertiesOffset = HeapObject::kHeaderSize;
  static const int kElementsOffset = kPropertiesOffset + kPointerSize;
  static const int kHeaderSize = kElementsOffset + kPointerSize;

  STATIC_ASSERT(kHeaderSize == Internals::kJSObjectHeaderSize);

  class BodyDescriptor : public FlexibleBodyDescriptor<kPropertiesOffset> {
   public:
    static inline int SizeOf(Map* map, HeapObject* object);
  };

  Context* GetCreationContext();

  // Enqueue change record for Object.observe. May cause GC.
  MUST_USE_RESULT static MaybeHandle<Object> EnqueueChangeRecord(
      Handle<JSObject> object, const char* type, Handle<Name> name,
      Handle<Object> old_value);

  // Gets the number of currently used elements.
  int GetFastElementsUsage();

  // Deletes an existing named property in a normalized object.
  static void DeleteNormalizedProperty(Handle<JSObject> object,
                                       Handle<Name> name, int entry);

  static bool AllCanRead(LookupIterator* it);
  static bool AllCanWrite(LookupIterator* it);

 private:
  friend class JSReceiver;
  friend class Object;

  static void MigrateFastToFast(Handle<JSObject> object, Handle<Map> new_map);
  static void MigrateFastToSlow(Handle<JSObject> object,
                                Handle<Map> new_map,
                                int expected_additional_properties);

  // Used from Object::GetProperty().
  MUST_USE_RESULT static MaybeHandle<Object> GetPropertyWithFailedAccessCheck(
      LookupIterator* it);

  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyWithFailedAccessCheck(
      LookupIterator* it, Handle<Object> value);

  // Add a property to a slow-case object.
  static void AddSlowProperty(Handle<JSObject> object,
                              Handle<Name> name,
                              Handle<Object> value,
                              PropertyAttributes attributes);

  MUST_USE_RESULT static MaybeHandle<Object> DeletePropertyWithInterceptor(
      LookupIterator* it);

  bool ReferencesObjectFromElements(FixedArray* elements,
                                    ElementsKind kind,
                                    Object* object);

  // Return the hash table backing store or the inline stored identity hash,
  // whatever is found.
  MUST_USE_RESULT Object* GetHiddenPropertiesHashTable();

  // Return the hash table backing store for hidden properties.  If there is no
  // backing store, allocate one.
  static Handle<ObjectHashTable> GetOrCreateHiddenPropertiesHashtable(
      Handle<JSObject> object);

  // Set the hidden property backing store to either a hash table or
  // the inline-stored identity hash.
  static Handle<Object> SetHiddenPropertiesHashTable(
      Handle<JSObject> object,
      Handle<Object> value);

  MUST_USE_RESULT Object* GetIdentityHash();

  static Handle<Smi> GetOrCreateIdentityHash(Handle<JSObject> object);

  static Handle<SeededNumberDictionary> GetNormalizedElementDictionary(
      Handle<JSObject> object, Handle<FixedArrayBase> elements);

  // Helper for fast versions of preventExtensions, seal, and freeze.
  // attrs is one of NONE, SEALED, or FROZEN (depending on the operation).
  template <PropertyAttributes attrs>
  MUST_USE_RESULT static MaybeHandle<Object> PreventExtensionsWithTransition(
      Handle<JSObject> object);

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSObject);
};


// Common superclass for FixedArrays that allow implementations to share
// common accessors and some code paths.
class FixedArrayBase: public HeapObject {
 public:
  // [length]: length of the array.
  inline int length() const;
  inline void set_length(int value);

  // Get and set the length using acquire loads and release stores.
  inline int synchronized_length() const;
  inline void synchronized_set_length(int value);

  DECLARE_CAST(FixedArrayBase)

  // Layout description.
  // Length is smi tagged when it is stored.
  static const int kLengthOffset = HeapObject::kHeaderSize;
  static const int kHeaderSize = kLengthOffset + kPointerSize;
};


class FixedDoubleArray;
class IncrementalMarking;


// FixedArray describes fixed-sized arrays with element type Object*.
class FixedArray: public FixedArrayBase {
 public:
  // Setter and getter for elements.
  inline Object* get(int index) const;
  static inline Handle<Object> get(Handle<FixedArray> array, int index);
  // Setter that uses write barrier.
  inline void set(int index, Object* value);
  inline bool is_the_hole(int index);

  // Setter that doesn't need write barrier.
  inline void set(int index, Smi* value);
  // Setter with explicit barrier mode.
  inline void set(int index, Object* value, WriteBarrierMode mode);

  // Setters for frequently used oddballs located in old space.
  inline void set_undefined(int index);
  inline void set_null(int index);
  inline void set_the_hole(int index);

  inline Object** GetFirstElementAddress();
  inline bool ContainsOnlySmisOrHoles();

  // Gives access to raw memory which stores the array's data.
  inline Object** data_start();

  inline void FillWithHoles(int from, int to);

  // Shrink length and insert filler objects.
  void Shrink(int length);

  enum KeyFilter { ALL_KEYS, NON_SYMBOL_KEYS };

  // Copy a sub array from the receiver to dest.
  void CopyTo(int pos, FixedArray* dest, int dest_pos, int len);

  // Garbage collection support.
  static int SizeFor(int length) { return kHeaderSize + length * kPointerSize; }

  // Code Generation support.
  static int OffsetOfElementAt(int index) { return SizeFor(index); }

  // Garbage collection support.
  inline Object** RawFieldOfElementAt(int index);

  DECLARE_CAST(FixedArray)

  // Maximal allowed size, in bytes, of a single FixedArray.
  // Prevents overflowing size computations, as well as extreme memory
  // consumption.
  static const int kMaxSize = 128 * MB * kPointerSize;
  // Maximally allowed length of a FixedArray.
  static const int kMaxLength = (kMaxSize - kHeaderSize) / kPointerSize;

  // Dispatched behavior.
  DECLARE_PRINTER(FixedArray)
  DECLARE_VERIFIER(FixedArray)
#ifdef DEBUG
  // Checks if two FixedArrays have identical contents.
  bool IsEqualTo(FixedArray* other);
#endif

  // Swap two elements in a pair of arrays.  If this array and the
  // numbers array are the same object, the elements are only swapped
  // once.
  void SwapPairs(FixedArray* numbers, int i, int j);

  // Sort prefix of this array and the numbers array as pairs wrt. the
  // numbers.  If the numbers array and the this array are the same
  // object, the prefix of this array is sorted.
  void SortPairs(FixedArray* numbers, uint32_t len);

  class BodyDescriptor : public FlexibleBodyDescriptor<kHeaderSize> {
   public:
    static inline int SizeOf(Map* map, HeapObject* object);
  };

 protected:
  // Set operation on FixedArray without using write barriers. Can
  // only be used for storing old space objects or smis.
  static inline void NoWriteBarrierSet(FixedArray* array,
                                       int index,
                                       Object* value);

  // Set operation on FixedArray without incremental write barrier. Can
  // only be used if the object is guaranteed to be white (whiteness witness
  // is present).
  static inline void NoIncrementalWriteBarrierSet(FixedArray* array,
                                                  int index,
                                                  Object* value);

 private:
  STATIC_ASSERT(kHeaderSize == Internals::kFixedArrayHeaderSize);

  DISALLOW_IMPLICIT_CONSTRUCTORS(FixedArray);
};


// FixedDoubleArray describes fixed-sized arrays with element type double.
class FixedDoubleArray: public FixedArrayBase {
 public:
  // Setter and getter for elements.
  inline double get_scalar(int index);
  inline uint64_t get_representation(int index);
  static inline Handle<Object> get(Handle<FixedDoubleArray> array, int index);
  inline void set(int index, double value);
  inline void set_the_hole(int index);

  // Checking for the hole.
  inline bool is_the_hole(int index);

  // Garbage collection support.
  inline static int SizeFor(int length) {
    return kHeaderSize + length * kDoubleSize;
  }

  // Gives access to raw memory which stores the array's data.
  inline double* data_start();

  inline void FillWithHoles(int from, int to);

  // Code Generation support.
  static int OffsetOfElementAt(int index) { return SizeFor(index); }

  DECLARE_CAST(FixedDoubleArray)

  // Maximal allowed size, in bytes, of a single FixedDoubleArray.
  // Prevents overflowing size computations, as well as extreme memory
  // consumption.
  static const int kMaxSize = 512 * MB;
  // Maximally allowed length of a FixedArray.
  static const int kMaxLength = (kMaxSize - kHeaderSize) / kDoubleSize;

  // Dispatched behavior.
  DECLARE_PRINTER(FixedDoubleArray)
  DECLARE_VERIFIER(FixedDoubleArray)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FixedDoubleArray);
};


class WeakFixedArray : public FixedArray {
 public:
  // If |maybe_array| is not a WeakFixedArray, a fresh one will be allocated.
  // This function does not check if the value exists already, callers must
  // ensure this themselves if necessary.
  static Handle<WeakFixedArray> Add(Handle<Object> maybe_array,
                                    Handle<HeapObject> value,
                                    int* assigned_index = NULL);

  // Returns true if an entry was found and removed.
  bool Remove(Handle<HeapObject> value);

  class NullCallback {
   public:
    static void Callback(Object* value, int old_index, int new_index) {}
  };

  template <class CompactionCallback>
  void Compact();

  inline Object* Get(int index) const;
  inline void Clear(int index);
  inline int Length() const;

  inline bool IsEmptySlot(int index) const;
  static Object* Empty() { return Smi::FromInt(0); }

  class Iterator {
   public:
    explicit Iterator(Object* maybe_array) : list_(NULL) { Reset(maybe_array); }
    void Reset(Object* maybe_array);

    template <class T>
    inline T* Next();

   private:
    int index_;
    WeakFixedArray* list_;
#ifdef DEBUG
    int last_used_index_;
    DisallowHeapAllocation no_gc_;
#endif  // DEBUG
    DISALLOW_COPY_AND_ASSIGN(Iterator);
  };

  DECLARE_CAST(WeakFixedArray)

 private:
  static const int kLastUsedIndexIndex = 0;
  static const int kFirstIndex = 1;

  static Handle<WeakFixedArray> Allocate(
      Isolate* isolate, int size, Handle<WeakFixedArray> initialize_from);

  static void Set(Handle<WeakFixedArray> array, int index,
                  Handle<HeapObject> value);
  inline void clear(int index);

  inline int last_used_index() const;
  inline void set_last_used_index(int index);

  // Disallow inherited setters.
  void set(int index, Smi* value);
  void set(int index, Object* value);
  void set(int index, Object* value, WriteBarrierMode mode);
  DISALLOW_IMPLICIT_CONSTRUCTORS(WeakFixedArray);
};


// Generic array grows dynamically with O(1) amortized insertion.
class ArrayList : public FixedArray {
 public:
  enum AddMode {
    kNone,
    // Use this if GC can delete elements from the array.
    kReloadLengthAfterAllocation,
  };
  static Handle<ArrayList> Add(Handle<ArrayList> array, Handle<Object> obj,
                               AddMode mode = kNone);
  static Handle<ArrayList> Add(Handle<ArrayList> array, Handle<Object> obj1,
                               Handle<Object> obj2, AddMode = kNone);
  inline int Length();
  inline void SetLength(int length);
  inline Object* Get(int index);
  inline Object** Slot(int index);
  inline void Set(int index, Object* obj);
  inline void Clear(int index, Object* undefined);
  DECLARE_CAST(ArrayList)

 private:
  static Handle<ArrayList> EnsureSpace(Handle<ArrayList> array, int length);
  static const int kLengthIndex = 0;
  static const int kFirstIndex = 1;
  DISALLOW_IMPLICIT_CONSTRUCTORS(ArrayList);
};


// DescriptorArrays are fixed arrays used to hold instance descriptors.
// The format of the these objects is:
//   [0]: Number of descriptors
//   [1]: Either Smi(0) if uninitialized, or a pointer to small fixed array:
//          [0]: pointer to fixed array with enum cache
//          [1]: either Smi(0) or pointer to fixed array with indices
//   [2]: first key
//   [2 + number of descriptors * kDescriptorSize]: start of slack
class DescriptorArray: public FixedArray {
 public:
  // Returns true for both shared empty_descriptor_array and for smis, which the
  // map uses to encode additional bit fields when the descriptor array is not
  // yet used.
  inline bool IsEmpty();

  // Returns the number of descriptors in the array.
  inline int number_of_descriptors();

  inline int number_of_descriptors_storage();

  inline int NumberOfSlackDescriptors();

  inline void SetNumberOfDescriptors(int number_of_descriptors);
  inline int number_of_entries();

  inline bool HasEnumCache();

  inline void CopyEnumCacheFrom(DescriptorArray* array);

  inline FixedArray* GetEnumCache();

  inline bool HasEnumIndicesCache();

  inline FixedArray* GetEnumIndicesCache();

  inline Object** GetEnumCacheSlot();

  void ClearEnumCache();

  // Initialize or change the enum cache,
  // using the supplied storage for the small "bridge".
  void SetEnumCache(FixedArray* bridge_storage,
                    FixedArray* new_cache,
                    Object* new_index_cache);

  bool CanHoldValue(int descriptor, Object* value);

  // Accessors for fetching instance descriptor at descriptor number.
  inline Name* GetKey(int descriptor_number);
  inline Object** GetKeySlot(int descriptor_number);
  inline Object* GetValue(int descriptor_number);
  inline void SetValue(int descriptor_number, Object* value);
  inline Object** GetValueSlot(int descriptor_number);
  static inline int GetValueOffset(int descriptor_number);
  inline Object** GetDescriptorStartSlot(int descriptor_number);
  inline Object** GetDescriptorEndSlot(int descriptor_number);
  inline PropertyDetails GetDetails(int descriptor_number);
  inline PropertyType GetType(int descriptor_number);
  inline int GetFieldIndex(int descriptor_number);
  inline HeapType* GetFieldType(int descriptor_number);
  inline Object* GetConstant(int descriptor_number);
  inline Object* GetCallbacksObject(int descriptor_number);
  inline AccessorDescriptor* GetCallbacks(int descriptor_number);

  inline Name* GetSortedKey(int descriptor_number);
  inline int GetSortedKeyIndex(int descriptor_number);
  inline void SetSortedKey(int pointer, int descriptor_number);
  inline void SetRepresentation(int descriptor_number,
                                Representation representation);

  // Accessor for complete descriptor.
  inline void Get(int descriptor_number, Descriptor* desc);
  inline void Set(int descriptor_number, Descriptor* desc);
  void Replace(int descriptor_number, Descriptor* descriptor);

  // Append automatically sets the enumeration index. This should only be used
  // to add descriptors in bulk at the end, followed by sorting the descriptor
  // array.
  inline void Append(Descriptor* desc);

  static Handle<DescriptorArray> CopyUpTo(Handle<DescriptorArray> desc,
                                          int enumeration_index,
                                          int slack = 0);

  static Handle<DescriptorArray> CopyUpToAddAttributes(
      Handle<DescriptorArray> desc,
      int enumeration_index,
      PropertyAttributes attributes,
      int slack = 0);

  // Sort the instance descriptors by the hash codes of their keys.
  void Sort();

  // Search the instance descriptors for given name.
  INLINE(int Search(Name* name, int number_of_own_descriptors));

  // As the above, but uses DescriptorLookupCache and updates it when
  // necessary.
  INLINE(int SearchWithCache(Name* name, Map* map));

  // Allocates a DescriptorArray, but returns the singleton
  // empty descriptor array object if number_of_descriptors is 0.
  static Handle<DescriptorArray> Allocate(Isolate* isolate,
                                          int number_of_descriptors,
                                          int slack = 0);

  DECLARE_CAST(DescriptorArray)

  // Constant for denoting key was not found.
  static const int kNotFound = -1;

  static const int kDescriptorLengthIndex = 0;
  static const int kEnumCacheIndex = 1;
  static const int kFirstIndex = 2;

  // The length of the "bridge" to the enum cache.
  static const int kEnumCacheBridgeLength = 2;
  static const int kEnumCacheBridgeCacheIndex = 0;
  static const int kEnumCacheBridgeIndicesCacheIndex = 1;

  // Layout description.
  static const int kDescriptorLengthOffset = FixedArray::kHeaderSize;
  static const int kEnumCacheOffset = kDescriptorLengthOffset + kPointerSize;
  static const int kFirstOffset = kEnumCacheOffset + kPointerSize;

  // Layout description for the bridge array.
  static const int kEnumCacheBridgeCacheOffset = FixedArray::kHeaderSize;

  // Layout of descriptor.
  static const int kDescriptorKey = 0;
  static const int kDescriptorDetails = 1;
  static const int kDescriptorValue = 2;
  static const int kDescriptorSize = 3;

#if defined(DEBUG) || defined(OBJECT_PRINT)
  // For our gdb macros, we should perhaps change these in the future.
  void Print();

  // Print all the descriptors.
  void PrintDescriptors(std::ostream& os);  // NOLINT
#endif

#ifdef DEBUG
  // Is the descriptor array sorted and without duplicates?
  bool IsSortedNoDuplicates(int valid_descriptors = -1);

  // Is the descriptor array consistent with the back pointers in targets?
  bool IsConsistentWithBackPointers(Map* current_map);

  // Are two DescriptorArrays equal?
  bool IsEqualTo(DescriptorArray* other);
#endif

  // Returns the fixed array length required to hold number_of_descriptors
  // descriptors.
  static int LengthFor(int number_of_descriptors) {
    return ToKeyIndex(number_of_descriptors);
  }

 private:
  // WhitenessWitness is used to prove that a descriptor array is white
  // (unmarked), so incremental write barriers can be skipped because the
  // marking invariant cannot be broken and slots pointing into evacuation
  // candidates will be discovered when the object is scanned. A witness is
  // always stack-allocated right after creating an array. By allocating a
  // witness, incremental marking is globally disabled. The witness is then
  // passed along wherever needed to statically prove that the array is known to
  // be white.
  class WhitenessWitness {
   public:
    inline explicit WhitenessWitness(DescriptorArray* array);
    inline ~WhitenessWitness();

   private:
    IncrementalMarking* marking_;
  };

  // An entry in a DescriptorArray, represented as an (array, index) pair.
  class Entry {
   public:
    inline explicit Entry(DescriptorArray* descs, int index) :
        descs_(descs), index_(index) { }

    inline PropertyType type();
    inline Object* GetCallbackObject();

   private:
    DescriptorArray* descs_;
    int index_;
  };

  // Conversion from descriptor number to array indices.
  static int ToKeyIndex(int descriptor_number) {
    return kFirstIndex +
           (descriptor_number * kDescriptorSize) +
           kDescriptorKey;
  }

  static int ToDetailsIndex(int descriptor_number) {
    return kFirstIndex +
           (descriptor_number * kDescriptorSize) +
           kDescriptorDetails;
  }

  static int ToValueIndex(int descriptor_number) {
    return kFirstIndex +
           (descriptor_number * kDescriptorSize) +
           kDescriptorValue;
  }

  // Transfer a complete descriptor from the src descriptor array to this
  // descriptor array.
  void CopyFrom(int index, DescriptorArray* src, const WhitenessWitness&);

  inline void Set(int descriptor_number,
                  Descriptor* desc,
                  const WhitenessWitness&);

  // Swap first and second descriptor.
  inline void SwapSortedKeys(int first, int second);

  DISALLOW_IMPLICIT_CONSTRUCTORS(DescriptorArray);
};


enum SearchMode { ALL_ENTRIES, VALID_ENTRIES };

template <SearchMode search_mode, typename T>
inline int Search(T* array, Name* name, int valid_entries = 0,
                  int* out_insertion_index = NULL);


// HashTable is a subclass of FixedArray that implements a hash table
// that uses open addressing and quadratic probing.
//
// In order for the quadratic probing to work, elements that have not
// yet been used and elements that have been deleted are
// distinguished.  Probing continues when deleted elements are
// encountered and stops when unused elements are encountered.
//
// - Elements with key == undefined have not been used yet.
// - Elements with key == the_hole have been deleted.
//
// The hash table class is parameterized with a Shape and a Key.
// Shape must be a class with the following interface:
//   class ExampleShape {
//    public:
//      // Tells whether key matches other.
//     static bool IsMatch(Key key, Object* other);
//     // Returns the hash value for key.
//     static uint32_t Hash(Key key);
//     // Returns the hash value for object.
//     static uint32_t HashForObject(Key key, Object* object);
//     // Convert key to an object.
//     static inline Handle<Object> AsHandle(Isolate* isolate, Key key);
//     // The prefix size indicates number of elements in the beginning
//     // of the backing storage.
//     static const int kPrefixSize = ..;
//     // The Element size indicates number of elements per entry.
//     static const int kEntrySize = ..;
//   };
// The prefix size indicates an amount of memory in the
// beginning of the backing storage that can be used for non-element
// information by subclasses.

template<typename Key>
class BaseShape {
 public:
  static const bool UsesSeed = false;
  static uint32_t Hash(Key key) { return 0; }
  static uint32_t SeededHash(Key key, uint32_t seed) {
    DCHECK(UsesSeed);
    return Hash(key);
  }
  static uint32_t HashForObject(Key key, Object* object) { return 0; }
  static uint32_t SeededHashForObject(Key key, uint32_t seed, Object* object) {
    DCHECK(UsesSeed);
    return HashForObject(key, object);
  }
};


class HashTableBase : public FixedArray {
 public:
  // Returns the number of elements in the hash table.
  inline int NumberOfElements();

  // Returns the number of deleted elements in the hash table.
  inline int NumberOfDeletedElements();

  // Returns the capacity of the hash table.
  inline int Capacity();

  // ElementAdded should be called whenever an element is added to a
  // hash table.
  inline void ElementAdded();

  // ElementRemoved should be called whenever an element is removed from
  // a hash table.
  inline void ElementRemoved();
  inline void ElementsRemoved(int n);

  // Computes the required capacity for a table holding the given
  // number of elements. May be more than HashTable::kMaxCapacity.
  static inline int ComputeCapacity(int at_least_space_for);

  // Tells whether k is a real key.  The hole and undefined are not allowed
  // as keys and can be used to indicate missing or deleted elements.
  inline bool IsKey(Object* k);

  // Compute the probe offset (quadratic probing).
  INLINE(static uint32_t GetProbeOffset(uint32_t n)) {
    return (n + n * n) >> 1;
  }

  static const int kNumberOfElementsIndex = 0;
  static const int kNumberOfDeletedElementsIndex = 1;
  static const int kCapacityIndex = 2;
  static const int kPrefixStartIndex = 3;

  // Constant used for denoting a absent entry.
  static const int kNotFound = -1;

 protected:
  // Update the number of elements in the hash table.
  inline void SetNumberOfElements(int nof);

  // Update the number of deleted elements in the hash table.
  inline void SetNumberOfDeletedElements(int nod);

  // Returns probe entry.
  static uint32_t GetProbe(uint32_t hash, uint32_t number, uint32_t size) {
    DCHECK(base::bits::IsPowerOfTwo32(size));
    return (hash + GetProbeOffset(number)) & (size - 1);
  }

  inline static uint32_t FirstProbe(uint32_t hash, uint32_t size) {
    return hash & (size - 1);
  }

  inline static uint32_t NextProbe(
      uint32_t last, uint32_t number, uint32_t size) {
    return (last + number) & (size - 1);
  }
};


template <typename Derived, typename Shape, typename Key>
class HashTable : public HashTableBase {
 public:
  // Wrapper methods
  inline uint32_t Hash(Key key) {
    if (Shape::UsesSeed) {
      return Shape::SeededHash(key, GetHeap()->HashSeed());
    } else {
      return Shape::Hash(key);
    }
  }

  inline uint32_t HashForObject(Key key, Object* object) {
    if (Shape::UsesSeed) {
      return Shape::SeededHashForObject(key, GetHeap()->HashSeed(), object);
    } else {
      return Shape::HashForObject(key, object);
    }
  }

  // Returns a new HashTable object.
  MUST_USE_RESULT static Handle<Derived> New(
      Isolate* isolate, int at_least_space_for,
      MinimumCapacity capacity_option = USE_DEFAULT_MINIMUM_CAPACITY,
      PretenureFlag pretenure = NOT_TENURED);

  DECLARE_CAST(HashTable)

  // Garbage collection support.
  void IteratePrefix(ObjectVisitor* visitor);
  void IterateElements(ObjectVisitor* visitor);

  // Find entry for key otherwise return kNotFound.
  inline int FindEntry(Key key);
  inline int FindEntry(Isolate* isolate, Key key, int32_t hash);
  int FindEntry(Isolate* isolate, Key key);

  // Rehashes the table in-place.
  void Rehash(Key key);

  // Returns the key at entry.
  Object* KeyAt(int entry) { return get(EntryToIndex(entry)); }

  static const int kElementsStartIndex = kPrefixStartIndex + Shape::kPrefixSize;
  static const int kEntrySize = Shape::kEntrySize;
  static const int kElementsStartOffset =
      kHeaderSize + kElementsStartIndex * kPointerSize;
  static const int kCapacityOffset =
      kHeaderSize + kCapacityIndex * kPointerSize;

  // Returns the index for an entry (of the key)
  static inline int EntryToIndex(int entry) {
    return (entry * kEntrySize) + kElementsStartIndex;
  }

 protected:
  friend class ObjectHashTable;

  // Find the entry at which to insert element with the given key that
  // has the given hash value.
  uint32_t FindInsertionEntry(uint32_t hash);

  // Attempt to shrink hash table after removal of key.
  MUST_USE_RESULT static Handle<Derived> Shrink(Handle<Derived> table, Key key);

  // Ensure enough space for n additional elements.
  MUST_USE_RESULT static Handle<Derived> EnsureCapacity(
      Handle<Derived> table,
      int n,
      Key key,
      PretenureFlag pretenure = NOT_TENURED);

  // Sets the capacity of the hash table.
  void SetCapacity(int capacity) {
    // To scale a computed hash code to fit within the hash table, we
    // use bit-wise AND with a mask, so the capacity must be positive
    // and non-zero.
    DCHECK(capacity > 0);
    DCHECK(capacity <= kMaxCapacity);
    set(kCapacityIndex, Smi::FromInt(capacity));
  }

  // Maximal capacity of HashTable. Based on maximal length of underlying
  // FixedArray. Staying below kMaxCapacity also ensures that EntryToIndex
  // cannot overflow.
  static const int kMaxCapacity =
      (FixedArray::kMaxLength - kElementsStartOffset) / kEntrySize;

 private:
  // Returns _expected_ if one of entries given by the first _probe_ probes is
  // equal to  _expected_. Otherwise, returns the entry given by the probe
  // number _probe_.
  uint32_t EntryForProbe(Key key, Object* k, int probe, uint32_t expected);

  void Swap(uint32_t entry1, uint32_t entry2, WriteBarrierMode mode);

  // Rehashes this hash-table into the new table.
  void Rehash(Handle<Derived> new_table, Key key);
};


// HashTableKey is an abstract superclass for virtual key behavior.
class HashTableKey {
 public:
  // Returns whether the other object matches this key.
  virtual bool IsMatch(Object* other) = 0;
  // Returns the hash value for this key.
  virtual uint32_t Hash() = 0;
  // Returns the hash value for object.
  virtual uint32_t HashForObject(Object* key) = 0;
  // Returns the key object for storing into the hash table.
  MUST_USE_RESULT virtual Handle<Object> AsHandle(Isolate* isolate) = 0;
  // Required.
  virtual ~HashTableKey() {}
};


class StringTableShape : public BaseShape<HashTableKey*> {
 public:
  static inline bool IsMatch(HashTableKey* key, Object* value) {
    return key->IsMatch(value);
  }

  static inline uint32_t Hash(HashTableKey* key) {
    return key->Hash();
  }

  static inline uint32_t HashForObject(HashTableKey* key, Object* object) {
    return key->HashForObject(object);
  }

  static inline Handle<Object> AsHandle(Isolate* isolate, HashTableKey* key);

  static const int kPrefixSize = 0;
  static const int kEntrySize = 1;
};

class SeqOneByteString;

// StringTable.
//
// No special elements in the prefix and the element size is 1
// because only the string itself (the key) needs to be stored.
class StringTable: public HashTable<StringTable,
                                    StringTableShape,
                                    HashTableKey*> {
 public:
  // Find string in the string table. If it is not there yet, it is
  // added. The return value is the string found.
  static Handle<String> LookupString(Isolate* isolate, Handle<String> key);
  static Handle<String> LookupKey(Isolate* isolate, HashTableKey* key);
  static String* LookupKeyIfExists(Isolate* isolate, HashTableKey* key);

  // Tries to internalize given string and returns string handle on success
  // or an empty handle otherwise.
  MUST_USE_RESULT static MaybeHandle<String> InternalizeStringIfExists(
      Isolate* isolate,
      Handle<String> string);

  // Looks up a string that is equal to the given string and returns
  // string handle if it is found, or an empty handle otherwise.
  MUST_USE_RESULT static MaybeHandle<String> LookupStringIfExists(
      Isolate* isolate,
      Handle<String> str);
  MUST_USE_RESULT static MaybeHandle<String> LookupTwoCharsStringIfExists(
      Isolate* isolate,
      uint16_t c1,
      uint16_t c2);

  static void EnsureCapacityForDeserialization(Isolate* isolate, int expected);

  DECLARE_CAST(StringTable)

 private:
  template <bool seq_one_byte>
  friend class JsonParser;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StringTable);
};


template <typename Derived, typename Shape, typename Key>
class Dictionary: public HashTable<Derived, Shape, Key> {
  typedef HashTable<Derived, Shape, Key> DerivedHashTable;

 public:
  // Returns the value at entry.
  Object* ValueAt(int entry) {
    return this->get(Derived::EntryToIndex(entry) + 1);
  }

  // Set the value for entry.
  void ValueAtPut(int entry, Object* value) {
    this->set(Derived::EntryToIndex(entry) + 1, value);
  }

  // Returns the property details for the property at entry.
  PropertyDetails DetailsAt(int entry) {
    return Shape::DetailsAt(static_cast<Derived*>(this), entry);
  }

  // Set the details for entry.
  void DetailsAtPut(int entry, PropertyDetails value) {
    Shape::DetailsAtPut(static_cast<Derived*>(this), entry, value);
  }

  // Returns true if property at given entry is deleted.
  bool IsDeleted(int entry) {
    return Shape::IsDeleted(static_cast<Derived*>(this), entry);
  }

  // Delete a property from the dictionary.
  static Handle<Object> DeleteProperty(Handle<Derived> dictionary, int entry);

  // Attempt to shrink the dictionary after deletion of key.
  MUST_USE_RESULT static inline Handle<Derived> Shrink(
      Handle<Derived> dictionary,
      Key key) {
    return DerivedHashTable::Shrink(dictionary, key);
  }

  // Sorting support
  // TODO(dcarney): templatize or move to SeededNumberDictionary
  void CopyValuesTo(FixedArray* elements);

  // Returns the number of elements in the dictionary filtering out properties
  // with the specified attributes.
  int NumberOfElementsFilterAttributes(PropertyAttributes filter);

  // Returns the number of enumerable elements in the dictionary.
  int NumberOfEnumElements() {
    return NumberOfElementsFilterAttributes(
        static_cast<PropertyAttributes>(DONT_ENUM | SYMBOLIC));
  }

  // Returns true if the dictionary contains any elements that are non-writable,
  // non-configurable, non-enumerable, or have getters/setters.
  bool HasComplexElements();

  enum SortMode { UNSORTED, SORTED };

  // Fill in details for properties into storage.
  // Returns the number of properties added.
  int CopyKeysTo(FixedArray* storage, int index, PropertyAttributes filter,
                 SortMode sort_mode);

  // Copies enumerable keys to preallocated fixed array.
  void CopyEnumKeysTo(FixedArray* storage);

  // Accessors for next enumeration index.
  void SetNextEnumerationIndex(int index) {
    DCHECK(index != 0);
    this->set(kNextEnumerationIndexIndex, Smi::FromInt(index));
  }

  int NextEnumerationIndex() {
    return Smi::cast(this->get(kNextEnumerationIndexIndex))->value();
  }

  // Creates a new dictionary.
  MUST_USE_RESULT static Handle<Derived> New(
      Isolate* isolate,
      int at_least_space_for,
      PretenureFlag pretenure = NOT_TENURED);

  // Ensure enough space for n additional elements.
  static Handle<Derived> EnsureCapacity(Handle<Derived> obj, int n, Key key);

#ifdef OBJECT_PRINT
  void Print(std::ostream& os);  // NOLINT
#endif
  // Returns the key (slow).
  Object* SlowReverseLookup(Object* value);

  // Sets the entry to (key, value) pair.
  inline void SetEntry(int entry,
                       Handle<Object> key,
                       Handle<Object> value);
  inline void SetEntry(int entry,
                       Handle<Object> key,
                       Handle<Object> value,
                       PropertyDetails details);

  MUST_USE_RESULT static Handle<Derived> Add(
      Handle<Derived> dictionary,
      Key key,
      Handle<Object> value,
      PropertyDetails details);

  // Returns iteration indices array for the |dictionary|.
  // Values are direct indices in the |HashTable| array.
  static Handle<FixedArray> BuildIterationIndicesArray(
      Handle<Derived> dictionary);

 protected:
  // Generic at put operation.
  MUST_USE_RESULT static Handle<Derived> AtPut(
      Handle<Derived> dictionary,
      Key key,
      Handle<Object> value);

  // Add entry to dictionary.
  static void AddEntry(
      Handle<Derived> dictionary,
      Key key,
      Handle<Object> value,
      PropertyDetails details,
      uint32_t hash);

  // Generate new enumeration indices to avoid enumeration index overflow.
  // Returns iteration indices array for the |dictionary|.
  static Handle<FixedArray> GenerateNewEnumerationIndices(
      Handle<Derived> dictionary);
  static const int kMaxNumberKeyIndex = DerivedHashTable::kPrefixStartIndex;
  static const int kNextEnumerationIndexIndex = kMaxNumberKeyIndex + 1;
};


template <typename Derived, typename Shape>
class NameDictionaryBase : public Dictionary<Derived, Shape, Handle<Name> > {
  typedef Dictionary<Derived, Shape, Handle<Name> > DerivedDictionary;

 public:
  // Find entry for key, otherwise return kNotFound. Optimized version of
  // HashTable::FindEntry.
  int FindEntry(Handle<Name> key);
};


template <typename Key>
class BaseDictionaryShape : public BaseShape<Key> {
 public:
  template <typename Dictionary>
  static inline PropertyDetails DetailsAt(Dictionary* dict, int entry) {
    STATIC_ASSERT(Dictionary::kEntrySize == 3);
    DCHECK(entry >= 0);  // Not found is -1, which is not caught by get().
    return PropertyDetails(
        Smi::cast(dict->get(Dictionary::EntryToIndex(entry) + 2)));
  }

  template <typename Dictionary>
  static inline void DetailsAtPut(Dictionary* dict, int entry,
                                  PropertyDetails value) {
    STATIC_ASSERT(Dictionary::kEntrySize == 3);
    dict->set(Dictionary::EntryToIndex(entry) + 2, value.AsSmi());
  }

  template <typename Dictionary>
  static bool IsDeleted(Dictionary* dict, int entry) {
    return false;
  }

  template <typename Dictionary>
  static inline void SetEntry(Dictionary* dict, int entry, Handle<Object> key,
                              Handle<Object> value, PropertyDetails details);
};


class NameDictionaryShape : public BaseDictionaryShape<Handle<Name> > {
 public:
  static inline bool IsMatch(Handle<Name> key, Object* other);
  static inline uint32_t Hash(Handle<Name> key);
  static inline uint32_t HashForObject(Handle<Name> key, Object* object);
  static inline Handle<Object> AsHandle(Isolate* isolate, Handle<Name> key);
  static const int kPrefixSize = 2;
  static const int kEntrySize = 3;
  static const bool kIsEnumerable = true;
};


class NameDictionary
    : public NameDictionaryBase<NameDictionary, NameDictionaryShape> {
  typedef NameDictionaryBase<NameDictionary, NameDictionaryShape>
      DerivedDictionary;

 public:
  DECLARE_CAST(NameDictionary)

  inline static Handle<FixedArray> DoGenerateNewEnumerationIndices(
      Handle<NameDictionary> dictionary);
};


class GlobalDictionaryShape : public NameDictionaryShape {
 public:
  static const int kEntrySize = 2;  // Overrides NameDictionaryShape::kEntrySize

  template <typename Dictionary>
  static inline PropertyDetails DetailsAt(Dictionary* dict, int entry);

  template <typename Dictionary>
  static inline void DetailsAtPut(Dictionary* dict, int entry,
                                  PropertyDetails value);

  template <typename Dictionary>
  static bool IsDeleted(Dictionary* dict, int entry);

  template <typename Dictionary>
  static inline void SetEntry(Dictionary* dict, int entry, Handle<Object> key,
                              Handle<Object> value, PropertyDetails details);
};


class GlobalDictionary
    : public NameDictionaryBase<GlobalDictionary, GlobalDictionaryShape> {
 public:
  DECLARE_CAST(GlobalDictionary)
};


class NumberDictionaryShape : public BaseDictionaryShape<uint32_t> {
 public:
  static inline bool IsMatch(uint32_t key, Object* other);
  static inline Handle<Object> AsHandle(Isolate* isolate, uint32_t key);
  static const int kEntrySize = 3;
  static const bool kIsEnumerable = false;
};


class SeededNumberDictionaryShape : public NumberDictionaryShape {
 public:
  static const bool UsesSeed = true;
  static const int kPrefixSize = 2;

  static inline uint32_t SeededHash(uint32_t key, uint32_t seed);
  static inline uint32_t SeededHashForObject(uint32_t key,
                                             uint32_t seed,
                                             Object* object);
};


class UnseededNumberDictionaryShape : public NumberDictionaryShape {
 public:
  static const int kPrefixSize = 0;

  static inline uint32_t Hash(uint32_t key);
  static inline uint32_t HashForObject(uint32_t key, Object* object);
};


class SeededNumberDictionary
    : public Dictionary<SeededNumberDictionary,
                        SeededNumberDictionaryShape,
                        uint32_t> {
 public:
  DECLARE_CAST(SeededNumberDictionary)

  // Type specific at put (default NONE attributes is used when adding).
  MUST_USE_RESULT static Handle<SeededNumberDictionary> AtNumberPut(
      Handle<SeededNumberDictionary> dictionary, uint32_t key,
      Handle<Object> value, bool used_as_prototype);
  MUST_USE_RESULT static Handle<SeededNumberDictionary> AddNumberEntry(
      Handle<SeededNumberDictionary> dictionary, uint32_t key,
      Handle<Object> value, PropertyDetails details, bool used_as_prototype);

  // Set an existing entry or add a new one if needed.
  // Return the updated dictionary.
  MUST_USE_RESULT static Handle<SeededNumberDictionary> Set(
      Handle<SeededNumberDictionary> dictionary, uint32_t key,
      Handle<Object> value, PropertyDetails details, bool used_as_prototype);

  void UpdateMaxNumberKey(uint32_t key, bool used_as_prototype);

  // If slow elements are required we will never go back to fast-case
  // for the elements kept in this dictionary.  We require slow
  // elements if an element has been added at an index larger than
  // kRequiresSlowElementsLimit or set_requires_slow_elements() has been called
  // when defining a getter or setter with a number key.
  inline bool requires_slow_elements();
  inline void set_requires_slow_elements();

  // Get the value of the max number key that has been added to this
  // dictionary.  max_number_key can only be called if
  // requires_slow_elements returns false.
  inline uint32_t max_number_key();

  // Bit masks.
  static const int kRequiresSlowElementsMask = 1;
  static const int kRequiresSlowElementsTagSize = 1;
  static const uint32_t kRequiresSlowElementsLimit = (1 << 29) - 1;
};


class UnseededNumberDictionary
    : public Dictionary<UnseededNumberDictionary,
                        UnseededNumberDictionaryShape,
                        uint32_t> {
 public:
  DECLARE_CAST(UnseededNumberDictionary)

  // Type specific at put (default NONE attributes is used when adding).
  MUST_USE_RESULT static Handle<UnseededNumberDictionary> AtNumberPut(
      Handle<UnseededNumberDictionary> dictionary,
      uint32_t key,
      Handle<Object> value);
  MUST_USE_RESULT static Handle<UnseededNumberDictionary> AddNumberEntry(
      Handle<UnseededNumberDictionary> dictionary,
      uint32_t key,
      Handle<Object> value);

  // Set an existing entry or add a new one if needed.
  // Return the updated dictionary.
  MUST_USE_RESULT static Handle<UnseededNumberDictionary> Set(
      Handle<UnseededNumberDictionary> dictionary,
      uint32_t key,
      Handle<Object> value);
};


class ObjectHashTableShape : public BaseShape<Handle<Object> > {
 public:
  static inline bool IsMatch(Handle<Object> key, Object* other);
  static inline uint32_t Hash(Handle<Object> key);
  static inline uint32_t HashForObject(Handle<Object> key, Object* object);
  static inline Handle<Object> AsHandle(Isolate* isolate, Handle<Object> key);
  static const int kPrefixSize = 0;
  static const int kEntrySize = 2;
};


// ObjectHashTable maps keys that are arbitrary objects to object values by
// using the identity hash of the key for hashing purposes.
class ObjectHashTable: public HashTable<ObjectHashTable,
                                        ObjectHashTableShape,
                                        Handle<Object> > {
  typedef HashTable<
      ObjectHashTable, ObjectHashTableShape, Handle<Object> > DerivedHashTable;
 public:
  DECLARE_CAST(ObjectHashTable)

  // Attempt to shrink hash table after removal of key.
  MUST_USE_RESULT static inline Handle<ObjectHashTable> Shrink(
      Handle<ObjectHashTable> table,
      Handle<Object> key);

  // Looks up the value associated with the given key. The hole value is
  // returned in case the key is not present.
  Object* Lookup(Handle<Object> key);
  Object* Lookup(Handle<Object> key, int32_t hash);
  Object* Lookup(Isolate* isolate, Handle<Object> key, int32_t hash);

  // Adds (or overwrites) the value associated with the given key.
  static Handle<ObjectHashTable> Put(Handle<ObjectHashTable> table,
                                     Handle<Object> key,
                                     Handle<Object> value);
  static Handle<ObjectHashTable> Put(Handle<ObjectHashTable> table,
                                     Handle<Object> key, Handle<Object> value,
                                     int32_t hash);

  // Returns an ObjectHashTable (possibly |table|) where |key| has been removed.
  static Handle<ObjectHashTable> Remove(Handle<ObjectHashTable> table,
                                        Handle<Object> key,
                                        bool* was_present);
  static Handle<ObjectHashTable> Remove(Handle<ObjectHashTable> table,
                                        Handle<Object> key, bool* was_present,
                                        int32_t hash);

 protected:
  friend class MarkCompactCollector;

  void AddEntry(int entry, Object* key, Object* value);
  void RemoveEntry(int entry);

  // Returns the index to the value of an entry.
  static inline int EntryToValueIndex(int entry) {
    return EntryToIndex(entry) + 1;
  }
};


// OrderedHashTable is a HashTable with Object keys that preserves
// insertion order. There are Map and Set interfaces (OrderedHashMap
// and OrderedHashTable, below). It is meant to be used by JSMap/JSSet.
//
// Only Object* keys are supported, with Object::SameValueZero() used as the
// equality operator and Object::GetHash() for the hash function.
//
// Based on the "Deterministic Hash Table" as described by Jason Orendorff at
// https://wiki.mozilla.org/User:Jorend/Deterministic_hash_tables
// Originally attributed to Tyler Close.
//
// Memory layout:
//   [0]: bucket count
//   [1]: element count
//   [2]: deleted element count
//   [3..(3 + NumberOfBuckets() - 1)]: "hash table", where each item is an
//                            offset into the data table (see below) where the
//                            first item in this bucket is stored.
//   [3 + NumberOfBuckets()..length]: "data table", an array of length
//                            Capacity() * kEntrySize, where the first entrysize
//                            items are handled by the derived class and the
//                            item at kChainOffset is another entry into the
//                            data table indicating the next entry in this hash
//                            bucket.
//
// When we transition the table to a new version we obsolete it and reuse parts
// of the memory to store information how to transition an iterator to the new
// table:
//
// Memory layout for obsolete table:
//   [0]: bucket count
//   [1]: Next newer table
//   [2]: Number of removed holes or -1 when the table was cleared.
//   [3..(3 + NumberOfRemovedHoles() - 1)]: The indexes of the removed holes.
//   [3 + NumberOfRemovedHoles()..length]: Not used
//
template<class Derived, class Iterator, int entrysize>
class OrderedHashTable: public FixedArray {
 public:
  // Returns an OrderedHashTable with a capacity of at least |capacity|.
  static Handle<Derived> Allocate(
      Isolate* isolate, int capacity, PretenureFlag pretenure = NOT_TENURED);

  // Returns an OrderedHashTable (possibly |table|) with enough space
  // to add at least one new element.
  static Handle<Derived> EnsureGrowable(Handle<Derived> table);

  // Returns an OrderedHashTable (possibly |table|) that's shrunken
  // if possible.
  static Handle<Derived> Shrink(Handle<Derived> table);

  // Returns a new empty OrderedHashTable and records the clearing so that
  // exisiting iterators can be updated.
  static Handle<Derived> Clear(Handle<Derived> table);

  // Returns a true if the OrderedHashTable contains the key
  static bool HasKey(Handle<Derived> table, Handle<Object> key);

  int NumberOfElements() {
    return Smi::cast(get(kNumberOfElementsIndex))->value();
  }

  int NumberOfDeletedElements() {
    return Smi::cast(get(kNumberOfDeletedElementsIndex))->value();
  }

  int UsedCapacity() { return NumberOfElements() + NumberOfDeletedElements(); }

  int NumberOfBuckets() {
    return Smi::cast(get(kNumberOfBucketsIndex))->value();
  }

  // Returns an index into |this| for the given entry.
  int EntryToIndex(int entry) {
    return kHashTableStartIndex + NumberOfBuckets() + (entry * kEntrySize);
  }

  int HashToBucket(int hash) { return hash & (NumberOfBuckets() - 1); }

  int HashToEntry(int hash) {
    int bucket = HashToBucket(hash);
    Object* entry = this->get(kHashTableStartIndex + bucket);
    return Smi::cast(entry)->value();
  }

  int KeyToFirstEntry(Object* key) {
    Object* hash = key->GetHash();
    // If the object does not have an identity hash, it was never used as a key
    if (hash->IsUndefined()) return kNotFound;
    return HashToEntry(Smi::cast(hash)->value());
  }

  int NextChainEntry(int entry) {
    Object* next_entry = get(EntryToIndex(entry) + kChainOffset);
    return Smi::cast(next_entry)->value();
  }

  Object* KeyAt(int entry) { return get(EntryToIndex(entry)); }

  bool IsObsolete() {
    return !get(kNextTableIndex)->IsSmi();
  }

  // The next newer table. This is only valid if the table is obsolete.
  Derived* NextTable() {
    return Derived::cast(get(kNextTableIndex));
  }

  // When the table is obsolete we store the indexes of the removed holes.
  int RemovedIndexAt(int index) {
    return Smi::cast(get(kRemovedHolesIndex + index))->value();
  }

  static const int kNotFound = -1;
  static const int kMinCapacity = 4;

  static const int kNumberOfBucketsIndex = 0;
  static const int kNumberOfElementsIndex = kNumberOfBucketsIndex + 1;
  static const int kNumberOfDeletedElementsIndex = kNumberOfElementsIndex + 1;
  static const int kHashTableStartIndex = kNumberOfDeletedElementsIndex + 1;
  static const int kNextTableIndex = kNumberOfElementsIndex;

  static const int kNumberOfBucketsOffset =
      kHeaderSize + kNumberOfBucketsIndex * kPointerSize;
  static const int kNumberOfElementsOffset =
      kHeaderSize + kNumberOfElementsIndex * kPointerSize;
  static const int kNumberOfDeletedElementsOffset =
      kHeaderSize + kNumberOfDeletedElementsIndex * kPointerSize;
  static const int kHashTableStartOffset =
      kHeaderSize + kHashTableStartIndex * kPointerSize;
  static const int kNextTableOffset =
      kHeaderSize + kNextTableIndex * kPointerSize;

  static const int kEntrySize = entrysize + 1;
  static const int kChainOffset = entrysize;

  static const int kLoadFactor = 2;

  // NumberOfDeletedElements is set to kClearedTableSentinel when
  // the table is cleared, which allows iterator transitions to
  // optimize that case.
  static const int kClearedTableSentinel = -1;

 protected:
  static Handle<Derived> Rehash(Handle<Derived> table, int new_capacity);

  void SetNumberOfBuckets(int num) {
    set(kNumberOfBucketsIndex, Smi::FromInt(num));
  }

  void SetNumberOfElements(int num) {
    set(kNumberOfElementsIndex, Smi::FromInt(num));
  }

  void SetNumberOfDeletedElements(int num) {
    set(kNumberOfDeletedElementsIndex, Smi::FromInt(num));
  }

  int Capacity() {
    return NumberOfBuckets() * kLoadFactor;
  }

  void SetNextTable(Derived* next_table) {
    set(kNextTableIndex, next_table);
  }

  void SetRemovedIndexAt(int index, int removed_index) {
    return set(kRemovedHolesIndex + index, Smi::FromInt(removed_index));
  }

  static const int kRemovedHolesIndex = kHashTableStartIndex;

  static const int kMaxCapacity =
      (FixedArray::kMaxLength - kHashTableStartIndex)
      / (1 + (kEntrySize * kLoadFactor));
};


class JSSetIterator;


class OrderedHashSet: public OrderedHashTable<
    OrderedHashSet, JSSetIterator, 1> {
 public:
  DECLARE_CAST(OrderedHashSet)

  static Handle<OrderedHashSet> Add(Handle<OrderedHashSet> table,
                                    Handle<Object> value);
};


class JSMapIterator;


class OrderedHashMap
    : public OrderedHashTable<OrderedHashMap, JSMapIterator, 2> {
 public:
  DECLARE_CAST(OrderedHashMap)

  inline Object* ValueAt(int entry);

  static const int kValueOffset = 1;
};


template <int entrysize>
class WeakHashTableShape : public BaseShape<Handle<Object> > {
 public:
  static inline bool IsMatch(Handle<Object> key, Object* other);
  static inline uint32_t Hash(Handle<Object> key);
  static inline uint32_t HashForObject(Handle<Object> key, Object* object);
  static inline Handle<Object> AsHandle(Isolate* isolate, Handle<Object> key);
  static const int kPrefixSize = 0;
  static const int kEntrySize = entrysize;
};


// WeakHashTable maps keys that are arbitrary heap objects to heap object
// values. The table wraps the keys in weak cells and store values directly.
// Thus it references keys weakly and values strongly.
class WeakHashTable: public HashTable<WeakHashTable,
                                      WeakHashTableShape<2>,
                                      Handle<Object> > {
  typedef HashTable<
      WeakHashTable, WeakHashTableShape<2>, Handle<Object> > DerivedHashTable;
 public:
  DECLARE_CAST(WeakHashTable)

  // Looks up the value associated with the given key. The hole value is
  // returned in case the key is not present.
  Object* Lookup(Handle<HeapObject> key);

  // Adds (or overwrites) the value associated with the given key. Mapping a
  // key to the hole value causes removal of the whole entry.
  MUST_USE_RESULT static Handle<WeakHashTable> Put(Handle<WeakHashTable> table,
                                                   Handle<HeapObject> key,
                                                   Handle<HeapObject> value);

  static Handle<FixedArray> GetValues(Handle<WeakHashTable> table);

 private:
  friend class MarkCompactCollector;

  void AddEntry(int entry, Handle<WeakCell> key, Handle<HeapObject> value);

  // Returns the index to the value of an entry.
  static inline int EntryToValueIndex(int entry) {
    return EntryToIndex(entry) + 1;
  }
};


// ScopeInfo represents information about different scopes of a source
// program  and the allocation of the scope's variables. Scope information
// is stored in a compressed form in ScopeInfo objects and is used
// at runtime (stack dumps, deoptimization, etc.).

// This object provides quick access to scope info details for runtime
// routines.
class ScopeInfo : public FixedArray {
 public:
  DECLARE_CAST(ScopeInfo)

  // Return the type of this scope.
  ScopeType scope_type();

  // Does this scope call eval?
  bool CallsEval();

  // Return the language mode of this scope.
  LanguageMode language_mode();

  // True if this scope is a (var) declaration scope.
  bool is_declaration_scope();

  // Does this scope make a sloppy eval call?
  bool CallsSloppyEval() { return CallsEval() && is_sloppy(language_mode()); }

  // Return the total number of locals allocated on the stack and in the
  // context. This includes the parameters that are allocated in the context.
  int LocalCount();

  // Return the number of stack slots for code. This number consists of two
  // parts:
  //  1. One stack slot per stack allocated local.
  //  2. One stack slot for the function name if it is stack allocated.
  int StackSlotCount();

  // Return the number of context slots for code if a context is allocated. This
  // number consists of three parts:
  //  1. Size of fixed header for every context: Context::MIN_CONTEXT_SLOTS
  //  2. One context slot per context allocated local.
  //  3. One context slot for the function name if it is context allocated.
  // Parameters allocated in the context count as context allocated locals. If
  // no contexts are allocated for this scope ContextLength returns 0.
  int ContextLength();

  // Does this scope declare a "this" binding?
  bool HasReceiver();

  // Does this scope declare a "this" binding, and the "this" binding is stack-
  // or context-allocated?
  bool HasAllocatedReceiver();

  // Is this scope the scope of a named function expression?
  bool HasFunctionName();

  // Return if this has context allocated locals.
  bool HasHeapAllocatedLocals();

  // Return if contexts are allocated for this scope.
  bool HasContext();

  // Return if this is a function scope with "use asm".
  inline bool IsAsmModule();

  // Return if this is a nested function within an asm module scope.
  inline bool IsAsmFunction();

  inline bool HasSimpleParameters();

  // Return the function_name if present.
  String* FunctionName();

  // Return the name of the given parameter.
  String* ParameterName(int var);

  // Return the name of the given local.
  String* LocalName(int var);

  // Return the name of the given stack local.
  String* StackLocalName(int var);

  // Return the name of the given stack local.
  int StackLocalIndex(int var);

  // Return the name of the given context local.
  String* ContextLocalName(int var);

  // Return the mode of the given context local.
  VariableMode ContextLocalMode(int var);

  // Return the initialization flag of the given context local.
  InitializationFlag ContextLocalInitFlag(int var);

  // Return the initialization flag of the given context local.
  MaybeAssignedFlag ContextLocalMaybeAssignedFlag(int var);

  // Return true if this local was introduced by the compiler, and should not be
  // exposed to the user in a debugger.
  bool LocalIsSynthetic(int var);

  String* StrongModeFreeVariableName(int var);
  int StrongModeFreeVariableStartPosition(int var);
  int StrongModeFreeVariableEndPosition(int var);

  // Lookup support for serialized scope info. Returns the
  // the stack slot index for a given slot name if the slot is
  // present; otherwise returns a value < 0. The name must be an internalized
  // string.
  int StackSlotIndex(String* name);

  // Lookup support for serialized scope info. Returns the local context slot
  // index for a given slot name if the slot is present; otherwise
  // returns a value < 0. The name must be an internalized string.
  // If the slot is present and mode != NULL, sets *mode to the corresponding
  // mode for that variable.
  static int ContextSlotIndex(Handle<ScopeInfo> scope_info, Handle<String> name,
                              VariableMode* mode, InitializationFlag* init_flag,
                              MaybeAssignedFlag* maybe_assigned_flag);

  // Similar to ContextSlotIndex() but this method searches only among
  // global slots of the serialized scope info. Returns the context slot index
  // for a given slot name if the slot is present; otherwise returns a
  // value < 0. The name must be an internalized string. If the slot is present
  // and mode != NULL, sets *mode to the corresponding mode for that variable.
  static int ContextGlobalSlotIndex(Handle<ScopeInfo> scope_info,
                                    Handle<String> name, VariableMode* mode,
                                    InitializationFlag* init_flag,
                                    MaybeAssignedFlag* maybe_assigned_flag);

  // Lookup the name of a certain context slot by its index.
  String* ContextSlotName(int slot_index);

  // Lookup support for serialized scope info. Returns the
  // parameter index for a given parameter name if the parameter is present;
  // otherwise returns a value < 0. The name must be an internalized string.
  int ParameterIndex(String* name);

  // Lookup support for serialized scope info. Returns the function context
  // slot index if the function name is present and context-allocated (named
  // function expressions, only), otherwise returns a value < 0. The name
  // must be an internalized string.
  int FunctionContextSlotIndex(String* name, VariableMode* mode);

  // Lookup support for serialized scope info.  Returns the receiver context
  // slot index if scope has a "this" binding, and the binding is
  // context-allocated.  Otherwise returns a value < 0.
  int ReceiverContextSlotIndex();

  FunctionKind function_kind();

  static Handle<ScopeInfo> Create(Isolate* isolate, Zone* zone, Scope* scope);
  static Handle<ScopeInfo> CreateGlobalThisBinding(Isolate* isolate);

  // Serializes empty scope info.
  static ScopeInfo* Empty(Isolate* isolate);

#ifdef DEBUG
  void Print();
#endif

  // The layout of the static part of a ScopeInfo is as follows. Each entry is
  // numeric and occupies one array slot.
  // 1. A set of properties of the scope
  // 2. The number of parameters. This only applies to function scopes. For
  //    non-function scopes this is 0.
  // 3. The number of non-parameter variables allocated on the stack.
  // 4. The number of non-parameter and parameter variables allocated in the
  //    context.
#define FOR_EACH_SCOPE_INFO_NUMERIC_FIELD(V) \
  V(Flags)                                   \
  V(ParameterCount)                          \
  V(StackLocalCount)                         \
  V(ContextLocalCount)                       \
  V(ContextGlobalCount)                      \
  V(StrongModeFreeVariableCount)

#define FIELD_ACCESSORS(name)       \
  inline void Set##name(int value); \
  inline int name();
  FOR_EACH_SCOPE_INFO_NUMERIC_FIELD(FIELD_ACCESSORS)
#undef FIELD_ACCESSORS

  enum {
#define DECL_INDEX(name) k##name,
    FOR_EACH_SCOPE_INFO_NUMERIC_FIELD(DECL_INDEX)
#undef DECL_INDEX
    kVariablePartIndex
  };

 private:
  // The layout of the variable part of a ScopeInfo is as follows:
  // 1. ParameterEntries:
  //    This part stores the names of the parameters for function scopes. One
  //    slot is used per parameter, so in total this part occupies
  //    ParameterCount() slots in the array. For other scopes than function
  //    scopes ParameterCount() is 0.
  // 2. StackLocalFirstSlot:
  //    Index of a first stack slot for stack local. Stack locals belonging to
  //    this scope are located on a stack at slots starting from this index.
  // 3. StackLocalEntries:
  //    Contains the names of local variables that are allocated on the stack,
  //    in increasing order of the stack slot index. First local variable has
  //    a stack slot index defined in StackLocalFirstSlot (point 2 above).
  //    One slot is used per stack local, so in total this part occupies
  //    StackLocalCount() slots in the array.
  // 4. ContextLocalNameEntries:
  //    Contains the names of local variables and parameters that are allocated
  //    in the context. They are stored in increasing order of the context slot
  //    index starting with Context::MIN_CONTEXT_SLOTS. One slot is used per
  //    context local, so in total this part occupies ContextLocalCount() slots
  //    in the array.
  // 5. ContextLocalInfoEntries:
  //    Contains the variable modes and initialization flags corresponding to
  //    the context locals in ContextLocalNameEntries. One slot is used per
  //    context local, so in total this part occupies ContextLocalCount()
  //    slots in the array.
  // 6. StrongModeFreeVariableNameEntries:
  //    Stores the names of strong mode free variables.
  // 7. StrongModeFreeVariablePositionEntries:
  //    Stores the locations (start and end position) of strong mode free
  //    variables.
  // 8. RecieverEntryIndex:
  //    If the scope binds a "this" value, one slot is reserved to hold the
  //    context or stack slot index for the variable.
  // 9. FunctionNameEntryIndex:
  //    If the scope belongs to a named function expression this part contains
  //    information about the function variable. It always occupies two array
  //    slots:  a. The name of the function variable.
  //            b. The context or stack slot index for the variable.
  int ParameterEntriesIndex();
  int StackLocalFirstSlotIndex();
  int StackLocalEntriesIndex();
  int ContextLocalNameEntriesIndex();
  int ContextGlobalNameEntriesIndex();
  int ContextLocalInfoEntriesIndex();
  int ContextGlobalInfoEntriesIndex();
  int StrongModeFreeVariableNameEntriesIndex();
  int StrongModeFreeVariablePositionEntriesIndex();
  int ReceiverEntryIndex();
  int FunctionNameEntryIndex();

  int Lookup(Handle<String> name, int start, int end, VariableMode* mode,
             VariableLocation* location, InitializationFlag* init_flag,
             MaybeAssignedFlag* maybe_assigned_flag);

  // Used for the function name variable for named function expressions, and for
  // the receiver.
  enum VariableAllocationInfo { NONE, STACK, CONTEXT, UNUSED };

  // Properties of scopes.
  class ScopeTypeField : public BitField<ScopeType, 0, 4> {};
  class CallsEvalField : public BitField<bool, ScopeTypeField::kNext, 1> {};
  STATIC_ASSERT(LANGUAGE_END == 3);
  class LanguageModeField
      : public BitField<LanguageMode, CallsEvalField::kNext, 2> {};
  class DeclarationScopeField
      : public BitField<bool, LanguageModeField::kNext, 1> {};
  class ReceiverVariableField
      : public BitField<VariableAllocationInfo, DeclarationScopeField::kNext,
                        2> {};
  class FunctionVariableField
      : public BitField<VariableAllocationInfo, ReceiverVariableField::kNext,
                        2> {};
  class FunctionVariableMode
      : public BitField<VariableMode, FunctionVariableField::kNext, 3> {};
  class AsmModuleField : public BitField<bool, FunctionVariableMode::kNext, 1> {
  };
  class AsmFunctionField : public BitField<bool, AsmModuleField::kNext, 1> {};
  class HasSimpleParametersField
      : public BitField<bool, AsmFunctionField::kNext, 1> {};
  class FunctionKindField
      : public BitField<FunctionKind, HasSimpleParametersField::kNext, 8> {};

  // BitFields representing the encoded information for context locals in the
  // ContextLocalInfoEntries part.
  class ContextLocalMode:      public BitField<VariableMode,         0, 3> {};
  class ContextLocalInitFlag:  public BitField<InitializationFlag,   3, 1> {};
  class ContextLocalMaybeAssignedFlag
      : public BitField<MaybeAssignedFlag, 4, 1> {};

  friend class ScopeIterator;
};


// The cache for maps used by normalized (dictionary mode) objects.
// Such maps do not have property descriptors, so a typical program
// needs very limited number of distinct normalized maps.
class NormalizedMapCache: public FixedArray {
 public:
  static Handle<NormalizedMapCache> New(Isolate* isolate);

  MUST_USE_RESULT MaybeHandle<Map> Get(Handle<Map> fast_map,
                                       PropertyNormalizationMode mode);
  void Set(Handle<Map> fast_map, Handle<Map> normalized_map);

  void Clear();

  DECLARE_CAST(NormalizedMapCache)

  static inline bool IsNormalizedMapCache(const Object* obj);

  DECLARE_VERIFIER(NormalizedMapCache)
 private:
  static const int kEntries = 64;

  static inline int GetIndex(Handle<Map> map);

  // The following declarations hide base class methods.
  Object* get(int index);
  void set(int index, Object* value);
};


// ByteArray represents fixed sized byte arrays.  Used for the relocation info
// that is attached to code objects.
class ByteArray: public FixedArrayBase {
 public:
  inline int Size();

  // Setter and getter.
  inline byte get(int index);
  inline void set(int index, byte value);

  // Treat contents as an int array.
  inline int get_int(int index);

  static int SizeFor(int length) {
    return OBJECT_POINTER_ALIGN(kHeaderSize + length);
  }
  // We use byte arrays for free blocks in the heap.  Given a desired size in
  // bytes that is a multiple of the word size and big enough to hold a byte
  // array, this function returns the number of elements a byte array should
  // have.
  static int LengthFor(int size_in_bytes) {
    DCHECK(IsAligned(size_in_bytes, kPointerSize));
    DCHECK(size_in_bytes >= kHeaderSize);
    return size_in_bytes - kHeaderSize;
  }

  // Returns data start address.
  inline Address GetDataStartAddress();

  // Returns a pointer to the ByteArray object for a given data start address.
  static inline ByteArray* FromDataStartAddress(Address address);

  DECLARE_CAST(ByteArray)

  // Dispatched behavior.
  inline int ByteArraySize();
  DECLARE_PRINTER(ByteArray)
  DECLARE_VERIFIER(ByteArray)

  // Layout description.
  static const int kAlignedSize = OBJECT_POINTER_ALIGN(kHeaderSize);

  // Maximal memory consumption for a single ByteArray.
  static const int kMaxSize = 512 * MB;
  // Maximal length of a single ByteArray.
  static const int kMaxLength = kMaxSize - kHeaderSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArray);
};


// BytecodeArray represents a sequence of interpreter bytecodes.
class BytecodeArray : public FixedArrayBase {
 public:
  static int SizeFor(int length) {
    return OBJECT_POINTER_ALIGN(kHeaderSize + length);
  }

  // Setter and getter
  inline byte get(int index);
  inline void set(int index, byte value);

  // Returns data start address.
  inline Address GetFirstBytecodeAddress();

  // Accessors for frame size.
  inline int frame_size() const;
  inline void set_frame_size(int frame_size);

  // Accessor for register count (derived from frame_size).
  inline int register_count() const;

  // Accessors for parameter count (including implicit 'this' receiver).
  inline int parameter_count() const;
  inline void set_parameter_count(int number_of_parameters);

  // Accessors for the constant pool.
  DECL_ACCESSORS(constant_pool, FixedArray)

  DECLARE_CAST(BytecodeArray)

  // Dispatched behavior.
  inline int BytecodeArraySize();
  inline void BytecodeArrayIterateBody(ObjectVisitor* v);

  DECLARE_PRINTER(BytecodeArray)
  DECLARE_VERIFIER(BytecodeArray)

  void Disassemble(std::ostream& os);

  // Layout description.
  static const int kFrameSizeOffset = FixedArrayBase::kHeaderSize;
  static const int kParameterSizeOffset = kFrameSizeOffset + kIntSize;
  static const int kConstantPoolOffset = kParameterSizeOffset + kIntSize;
  static const int kHeaderSize = kConstantPoolOffset + kPointerSize;

  static const int kAlignedSize = OBJECT_POINTER_ALIGN(kHeaderSize);

  // Maximal memory consumption for a single BytecodeArray.
  static const int kMaxSize = 512 * MB;
  // Maximal length of a single BytecodeArray.
  static const int kMaxLength = kMaxSize - kHeaderSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArray);
};


// FreeSpace are fixed-size free memory blocks used by the heap and GC.
// They look like heap objects (are heap object tagged and have a map) so that
// the heap remains iterable.  They have a size and a next pointer.
// The next pointer is the raw address of the next FreeSpace object (or NULL)
// in the free list.
class FreeSpace: public HeapObject {
 public:
  // [size]: size of the free space including the header.
  inline int size() const;
  inline void set_size(int value);

  inline int nobarrier_size() const;
  inline void nobarrier_set_size(int value);

  inline int Size();

  // Accessors for the next field.
  inline FreeSpace* next();
  inline FreeSpace** next_address();
  inline void set_next(FreeSpace* next);

  inline static FreeSpace* cast(HeapObject* obj);

  // Dispatched behavior.
  DECLARE_PRINTER(FreeSpace)
  DECLARE_VERIFIER(FreeSpace)

  // Layout description.
  // Size is smi tagged when it is stored.
  static const int kSizeOffset = HeapObject::kHeaderSize;
  static const int kNextOffset = POINTER_SIZE_ALIGN(kSizeOffset + kPointerSize);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FreeSpace);
};


// V has parameters (Type, type, TYPE, C type, element_size)
#define TYPED_ARRAYS(V) \
  V(Uint8, uint8, UINT8, uint8_t, 1)                                           \
  V(Int8, int8, INT8, int8_t, 1)                                               \
  V(Uint16, uint16, UINT16, uint16_t, 2)                                       \
  V(Int16, int16, INT16, int16_t, 2)                                           \
  V(Uint32, uint32, UINT32, uint32_t, 4)                                       \
  V(Int32, int32, INT32, int32_t, 4)                                           \
  V(Float32, float32, FLOAT32, float, 4)                                       \
  V(Float64, float64, FLOAT64, double, 8)                                      \
  V(Uint8Clamped, uint8_clamped, UINT8_CLAMPED, uint8_t, 1)


class FixedTypedArrayBase: public FixedArrayBase {
 public:
  // [base_pointer]: Either points to the FixedTypedArrayBase itself or nullptr.
  DECL_ACCESSORS(base_pointer, Object)

  // [external_pointer]: Contains the offset between base_pointer and the start
  // of the data. If the base_pointer is a nullptr, the external_pointer
  // therefore points to the actual backing store.
  DECL_ACCESSORS(external_pointer, void)

  // Dispatched behavior.
  inline void FixedTypedArrayBaseIterateBody(ObjectVisitor* v);

  template <typename StaticVisitor>
  inline void FixedTypedArrayBaseIterateBody();

  DECLARE_CAST(FixedTypedArrayBase)

  static const int kBasePointerOffset = FixedArrayBase::kHeaderSize;
  static const int kExternalPointerOffset = kBasePointerOffset + kPointerSize;
  static const int kHeaderSize =
      DOUBLE_POINTER_ALIGN(kExternalPointerOffset + kPointerSize);

  static const int kDataOffset = kHeaderSize;

  inline int size();

  static inline int TypedArraySize(InstanceType type, int length);
  inline int TypedArraySize(InstanceType type);

  // Use with care: returns raw pointer into heap.
  inline void* DataPtr();

  inline int DataSize();

 private:
  static inline int ElementSize(InstanceType type);

  inline int DataSize(InstanceType type);

  DISALLOW_IMPLICIT_CONSTRUCTORS(FixedTypedArrayBase);
};


template <class Traits>
class FixedTypedArray: public FixedTypedArrayBase {
 public:
  typedef typename Traits::ElementType ElementType;
  static const InstanceType kInstanceType = Traits::kInstanceType;

  DECLARE_CAST(FixedTypedArray<Traits>)

  inline ElementType get_scalar(int index);
  static inline Handle<Object> get(Handle<FixedTypedArray> array, int index);
  inline void set(int index, ElementType value);

  static inline ElementType from_int(int value);
  static inline ElementType from_double(double value);

  // This accessor applies the correct conversion from Smi, HeapNumber
  // and undefined.
  inline void SetValue(uint32_t index, Object* value);

  DECLARE_PRINTER(FixedTypedArray)
  DECLARE_VERIFIER(FixedTypedArray)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FixedTypedArray);
};

#define FIXED_TYPED_ARRAY_TRAITS(Type, type, TYPE, elementType, size)         \
  class Type##ArrayTraits {                                                   \
   public:   /* NOLINT */                                                     \
    typedef elementType ElementType;                                          \
    static const InstanceType kInstanceType = FIXED_##TYPE##_ARRAY_TYPE;      \
    static const char* Designator() { return #type " array"; }                \
    static inline Handle<Object> ToHandle(Isolate* isolate,                   \
                                          elementType scalar);                \
    static inline elementType defaultValue();                                 \
  };                                                                          \
                                                                              \
  typedef FixedTypedArray<Type##ArrayTraits> Fixed##Type##Array;

TYPED_ARRAYS(FIXED_TYPED_ARRAY_TRAITS)

#undef FIXED_TYPED_ARRAY_TRAITS


// DeoptimizationInputData is a fixed array used to hold the deoptimization
// data for code generated by the Hydrogen/Lithium compiler.  It also
// contains information about functions that were inlined.  If N different
// functions were inlined then first N elements of the literal array will
// contain these functions.
//
// It can be empty.
class DeoptimizationInputData: public FixedArray {
 public:
  // Layout description.  Indices in the array.
  static const int kTranslationByteArrayIndex = 0;
  static const int kInlinedFunctionCountIndex = 1;
  static const int kLiteralArrayIndex = 2;
  static const int kOsrAstIdIndex = 3;
  static const int kOsrPcOffsetIndex = 4;
  static const int kOptimizationIdIndex = 5;
  static const int kSharedFunctionInfoIndex = 6;
  static const int kWeakCellCacheIndex = 7;
  static const int kFirstDeoptEntryIndex = 8;

  // Offsets of deopt entry elements relative to the start of the entry.
  static const int kAstIdRawOffset = 0;
  static const int kTranslationIndexOffset = 1;
  static const int kArgumentsStackHeightOffset = 2;
  static const int kPcOffset = 3;
  static const int kDeoptEntrySize = 4;

  // Simple element accessors.
#define DECLARE_ELEMENT_ACCESSORS(name, type) \
  inline type* name();                        \
  inline void Set##name(type* value);

  DECLARE_ELEMENT_ACCESSORS(TranslationByteArray, ByteArray)
  DECLARE_ELEMENT_ACCESSORS(InlinedFunctionCount, Smi)
  DECLARE_ELEMENT_ACCESSORS(LiteralArray, FixedArray)
  DECLARE_ELEMENT_ACCESSORS(OsrAstId, Smi)
  DECLARE_ELEMENT_ACCESSORS(OsrPcOffset, Smi)
  DECLARE_ELEMENT_ACCESSORS(OptimizationId, Smi)
  DECLARE_ELEMENT_ACCESSORS(SharedFunctionInfo, Object)
  DECLARE_ELEMENT_ACCESSORS(WeakCellCache, Object)

#undef DECLARE_ELEMENT_ACCESSORS

  // Accessors for elements of the ith deoptimization entry.
#define DECLARE_ENTRY_ACCESSORS(name, type) \
  inline type* name(int i);                 \
  inline void Set##name(int i, type* value);

  DECLARE_ENTRY_ACCESSORS(AstIdRaw, Smi)
  DECLARE_ENTRY_ACCESSORS(TranslationIndex, Smi)
  DECLARE_ENTRY_ACCESSORS(ArgumentsStackHeight, Smi)
  DECLARE_ENTRY_ACCESSORS(Pc, Smi)

#undef DECLARE_ENTRY_ACCESSORS

  inline BailoutId AstId(int i);

  inline void SetAstId(int i, BailoutId value);

  inline int DeoptCount();

  // Allocates a DeoptimizationInputData.
  static Handle<DeoptimizationInputData> New(Isolate* isolate,
                                             int deopt_entry_count,
                                             PretenureFlag pretenure);

  DECLARE_CAST(DeoptimizationInputData)

#ifdef ENABLE_DISASSEMBLER
  void DeoptimizationInputDataPrint(std::ostream& os);  // NOLINT
#endif

 private:
  static int IndexForEntry(int i) {
    return kFirstDeoptEntryIndex + (i * kDeoptEntrySize);
  }


  static int LengthFor(int entry_count) { return IndexForEntry(entry_count); }
};


// DeoptimizationOutputData is a fixed array used to hold the deoptimization
// data for code generated by the full compiler.
// The format of the these objects is
//   [i * 2]: Ast ID for ith deoptimization.
//   [i * 2 + 1]: PC and state of ith deoptimization
class DeoptimizationOutputData: public FixedArray {
 public:
  inline int DeoptPoints();

  inline BailoutId AstId(int index);

  inline void SetAstId(int index, BailoutId id);

  inline Smi* PcAndState(int index);
  inline void SetPcAndState(int index, Smi* offset);

  static int LengthOfFixedArray(int deopt_points) {
    return deopt_points * 2;
  }

  // Allocates a DeoptimizationOutputData.
  static Handle<DeoptimizationOutputData> New(Isolate* isolate,
                                              int number_of_deopt_points,
                                              PretenureFlag pretenure);

  DECLARE_CAST(DeoptimizationOutputData)

#if defined(OBJECT_PRINT) || defined(ENABLE_DISASSEMBLER)
  void DeoptimizationOutputDataPrint(std::ostream& os);  // NOLINT
#endif
};


// HandlerTable is a fixed array containing entries for exception handlers in
// the code object it is associated with. The tables comes in two flavors:
// 1) Based on ranges: Used for unoptimized code. Contains one entry per
//    exception handler and a range representing the try-block covered by that
//    handler. Layout looks as follows:
//      [ range-start , range-end , handler-offset , stack-depth ]
// 2) Based on return addresses: Used for turbofanned code. Contains one entry
//    per call-site that could throw an exception. Layout looks as follows:
//      [ return-address-offset , handler-offset ]
class HandlerTable : public FixedArray {
 public:
  // Conservative prediction whether a given handler will locally catch an
  // exception or cause a re-throw to outside the code boundary. Since this is
  // undecidable it is merely an approximation (e.g. useful for debugger).
  enum CatchPrediction { UNCAUGHT, CAUGHT };

  // Accessors for handler table based on ranges.
  inline void SetRangeStart(int index, int value);
  inline void SetRangeEnd(int index, int value);
  inline void SetRangeHandler(int index, int offset, CatchPrediction pred);
  inline void SetRangeDepth(int index, int value);

  // Accessors for handler table based on return addresses.
  inline void SetReturnOffset(int index, int value);
  inline void SetReturnHandler(int index, int offset, CatchPrediction pred);

  // Lookup handler in a table based on ranges.
  int LookupRange(int pc_offset, int* stack_depth, CatchPrediction* prediction);

  // Lookup handler in a table based on return addresses.
  int LookupReturn(int pc_offset, CatchPrediction* prediction);

  // Returns the required length of the underlying fixed array.
  static int LengthForRange(int entries) { return entries * kRangeEntrySize; }
  static int LengthForReturn(int entries) { return entries * kReturnEntrySize; }

  DECLARE_CAST(HandlerTable)

#if defined(OBJECT_PRINT) || defined(ENABLE_DISASSEMBLER)
  void HandlerTableRangePrint(std::ostream& os);   // NOLINT
  void HandlerTableReturnPrint(std::ostream& os);  // NOLINT
#endif

 private:
  // Layout description for handler table based on ranges.
  static const int kRangeStartIndex = 0;
  static const int kRangeEndIndex = 1;
  static const int kRangeHandlerIndex = 2;
  static const int kRangeDepthIndex = 3;
  static const int kRangeEntrySize = 4;

  // Layout description for handler table based on return addresses.
  static const int kReturnOffsetIndex = 0;
  static const int kReturnHandlerIndex = 1;
  static const int kReturnEntrySize = 2;

  // Encoding of the {handler} field.
  class HandlerPredictionField : public BitField<CatchPrediction, 0, 1> {};
  class HandlerOffsetField : public BitField<int, 1, 30> {};
};


// Code describes objects with on-the-fly generated machine code.
class Code: public HeapObject {
 public:
  // Opaque data type for encapsulating code flags like kind, inline
  // cache state, and arguments count.
  typedef uint32_t Flags;

#define NON_IC_KIND_LIST(V) \
  V(FUNCTION)               \
  V(OPTIMIZED_FUNCTION)     \
  V(STUB)                   \
  V(HANDLER)                \
  V(BUILTIN)                \
  V(REGEXP)                 \
  V(WASM_FUNCTION)

#define IC_KIND_LIST(V) \
  V(LOAD_IC)            \
  V(KEYED_LOAD_IC)      \
  V(CALL_IC)            \
  V(STORE_IC)           \
  V(KEYED_STORE_IC)     \
  V(BINARY_OP_IC)       \
  V(COMPARE_IC)         \
  V(COMPARE_NIL_IC)     \
  V(TO_BOOLEAN_IC)

#define CODE_KIND_LIST(V) \
  NON_IC_KIND_LIST(V)     \
  IC_KIND_LIST(V)

  enum Kind {
#define DEFINE_CODE_KIND_ENUM(name) name,
    CODE_KIND_LIST(DEFINE_CODE_KIND_ENUM)
#undef DEFINE_CODE_KIND_ENUM
    NUMBER_OF_KINDS
  };

  // No more than 16 kinds. The value is currently encoded in four bits in
  // Flags.
  STATIC_ASSERT(NUMBER_OF_KINDS <= 16);

  static const char* Kind2String(Kind kind);

  // Types of stubs.
  enum StubType {
    NORMAL,
    FAST
  };

  static const int kPrologueOffsetNotSet = -1;

#ifdef ENABLE_DISASSEMBLER
  // Printing
  static const char* ICState2String(InlineCacheState state);
  static const char* StubType2String(StubType type);
  static void PrintExtraICState(std::ostream& os,  // NOLINT
                                Kind kind, ExtraICState extra);
  void Disassemble(const char* name, std::ostream& os);  // NOLINT
#endif  // ENABLE_DISASSEMBLER

  // [instruction_size]: Size of the native instructions
  inline int instruction_size() const;
  inline void set_instruction_size(int value);

  // [relocation_info]: Code relocation information
  DECL_ACCESSORS(relocation_info, ByteArray)
  void InvalidateRelocation();
  void InvalidateEmbeddedObjects();

  // [handler_table]: Fixed array containing offsets of exception handlers.
  DECL_ACCESSORS(handler_table, FixedArray)

  // [deoptimization_data]: Array containing data for deopt.
  DECL_ACCESSORS(deoptimization_data, FixedArray)

  // [raw_type_feedback_info]: This field stores various things, depending on
  // the kind of the code object.
  //   FUNCTION           => type feedback information.
  //   STUB and ICs       => major/minor key as Smi.
  DECL_ACCESSORS(raw_type_feedback_info, Object)
  inline Object* type_feedback_info();
  inline void set_type_feedback_info(
      Object* value, WriteBarrierMode mode = UPDATE_WRITE_BARRIER);
  inline uint32_t stub_key();
  inline void set_stub_key(uint32_t key);

  // [next_code_link]: Link for lists of optimized or deoptimized code.
  // Note that storage for this field is overlapped with typefeedback_info.
  DECL_ACCESSORS(next_code_link, Object)

  // [gc_metadata]: Field used to hold GC related metadata. The contents of this
  // field does not have to be traced during garbage collection since
  // it is only used by the garbage collector itself.
  DECL_ACCESSORS(gc_metadata, Object)

  // [ic_age]: Inline caching age: the value of the Heap::global_ic_age
  // at the moment when this object was created.
  inline void set_ic_age(int count);
  inline int ic_age() const;

  // [prologue_offset]: Offset of the function prologue, used for aging
  // FUNCTIONs and OPTIMIZED_FUNCTIONs.
  inline int prologue_offset() const;
  inline void set_prologue_offset(int offset);

  // [constant_pool offset]: Offset of the constant pool.
  // Valid for FLAG_enable_embedded_constant_pool only
  inline int constant_pool_offset() const;
  inline void set_constant_pool_offset(int offset);

  // Unchecked accessors to be used during GC.
  inline ByteArray* unchecked_relocation_info();

  inline int relocation_size();

  // [flags]: Various code flags.
  inline Flags flags();
  inline void set_flags(Flags flags);

  // [flags]: Access to specific code flags.
  inline Kind kind();
  inline InlineCacheState ic_state();  // Only valid for IC stubs.
  inline ExtraICState extra_ic_state();  // Only valid for IC stubs.

  inline StubType type();  // Only valid for monomorphic IC stubs.

  // Testers for IC stub kinds.
  inline bool is_inline_cache_stub();
  inline bool is_debug_stub();
  inline bool is_handler();
  inline bool is_load_stub();
  inline bool is_keyed_load_stub();
  inline bool is_store_stub();
  inline bool is_keyed_store_stub();
  inline bool is_call_stub();
  inline bool is_binary_op_stub();
  inline bool is_compare_ic_stub();
  inline bool is_compare_nil_ic_stub();
  inline bool is_to_boolean_ic_stub();
  inline bool is_keyed_stub();
  inline bool is_optimized_code();
  inline bool embeds_maps_weakly();

  inline bool IsCodeStubOrIC();
  inline bool IsJavaScriptCode();

  inline void set_raw_kind_specific_flags1(int value);
  inline void set_raw_kind_specific_flags2(int value);

  // [is_crankshafted]: For kind STUB or ICs, tells whether or not a code
  // object was generated by either the hydrogen or the TurboFan optimizing
  // compiler (but it may not be an optimized function).
  inline bool is_crankshafted();
  inline bool is_hydrogen_stub();  // Crankshafted, but not a function.
  inline void set_is_crankshafted(bool value);

  // [is_turbofanned]: For kind STUB or OPTIMIZED_FUNCTION, tells whether the
  // code object was generated by the TurboFan optimizing compiler.
  inline bool is_turbofanned();
  inline void set_is_turbofanned(bool value);

  // [can_have_weak_objects]: For kind OPTIMIZED_FUNCTION, tells whether the
  // embedded objects in code should be treated weakly.
  inline bool can_have_weak_objects();
  inline void set_can_have_weak_objects(bool value);

  // [has_deoptimization_support]: For FUNCTION kind, tells if it has
  // deoptimization support.
  inline bool has_deoptimization_support();
  inline void set_has_deoptimization_support(bool value);

  // [has_debug_break_slots]: For FUNCTION kind, tells if it has
  // been compiled with debug break slots.
  inline bool has_debug_break_slots();
  inline void set_has_debug_break_slots(bool value);

  // [has_reloc_info_for_serialization]: For FUNCTION kind, tells if its
  // reloc info includes runtime and external references to support
  // serialization/deserialization.
  inline bool has_reloc_info_for_serialization();
  inline void set_has_reloc_info_for_serialization(bool value);

  // [allow_osr_at_loop_nesting_level]: For FUNCTION kind, tells for
  // how long the function has been marked for OSR and therefore which
  // level of loop nesting we are willing to do on-stack replacement
  // for.
  inline void set_allow_osr_at_loop_nesting_level(int level);
  inline int allow_osr_at_loop_nesting_level();

  // [profiler_ticks]: For FUNCTION kind, tells for how many profiler ticks
  // the code object was seen on the stack with no IC patching going on.
  inline int profiler_ticks();
  inline void set_profiler_ticks(int ticks);

  // [builtin_index]: For BUILTIN kind, tells which builtin index it has.
  // For builtins, tells which builtin index it has.
  // Note that builtins can have a code kind other than BUILTIN, which means
  // that for arbitrary code objects, this index value may be random garbage.
  // To verify in that case, compare the code object to the indexed builtin.
  inline int builtin_index();
  inline void set_builtin_index(int id);

  // [stack_slots]: For kind OPTIMIZED_FUNCTION, the number of stack slots
  // reserved in the code prologue.
  inline unsigned stack_slots();
  inline void set_stack_slots(unsigned slots);

  // [safepoint_table_start]: For kind OPTIMIZED_FUNCTION, the offset in
  // the instruction stream where the safepoint table starts.
  inline unsigned safepoint_table_offset();
  inline void set_safepoint_table_offset(unsigned offset);

  // [back_edge_table_start]: For kind FUNCTION, the offset in the
  // instruction stream where the back edge table starts.
  inline unsigned back_edge_table_offset();
  inline void set_back_edge_table_offset(unsigned offset);

  inline bool back_edges_patched_for_osr();

  // [to_boolean_foo]: For kind TO_BOOLEAN_IC tells what state the stub is in.
  inline uint16_t to_boolean_state();

  // [has_function_cache]: For kind STUB tells whether there is a function
  // cache is passed to the stub.
  inline bool has_function_cache();
  inline void set_has_function_cache(bool flag);


  // [marked_for_deoptimization]: For kind OPTIMIZED_FUNCTION tells whether
  // the code is going to be deoptimized because of dead embedded maps.
  inline bool marked_for_deoptimization();
  inline void set_marked_for_deoptimization(bool flag);

  // [constant_pool]: The constant pool for this function.
  inline Address constant_pool();

  // Get the safepoint entry for the given pc.
  SafepointEntry GetSafepointEntry(Address pc);

  // Find an object in a stub with a specified map
  Object* FindNthObject(int n, Map* match_map);

  // Find the first allocation site in an IC stub.
  AllocationSite* FindFirstAllocationSite();

  // Find the first map in an IC stub.
  Map* FindFirstMap();
  void FindAllMaps(MapHandleList* maps);

  // Find the first handler in an IC stub.
  Code* FindFirstHandler();

  // Find |length| handlers and put them into |code_list|. Returns false if not
  // enough handlers can be found.
  bool FindHandlers(CodeHandleList* code_list, int length = -1);

  // Find the handler for |map|.
  MaybeHandle<Code> FindHandlerForMap(Map* map);

  // Find the first name in an IC stub.
  Name* FindFirstName();

  class FindAndReplacePattern;
  // For each (map-to-find, object-to-replace) pair in the pattern, this
  // function replaces the corresponding placeholder in the code with the
  // object-to-replace. The function assumes that pairs in the pattern come in
  // the same order as the placeholders in the code.
  // If the placeholder is a weak cell, then the value of weak cell is matched
  // against the map-to-find.
  void FindAndReplace(const FindAndReplacePattern& pattern);

  // The entire code object including its header is copied verbatim to the
  // snapshot so that it can be written in one, fast, memcpy during
  // deserialization. The deserializer will overwrite some pointers, rather
  // like a runtime linker, but the random allocation addresses used in the
  // mksnapshot process would still be present in the unlinked snapshot data,
  // which would make snapshot production non-reproducible. This method wipes
  // out the to-be-overwritten header data for reproducible snapshots.
  inline void WipeOutHeader();

  // Flags operations.
  static inline Flags ComputeFlags(
      Kind kind, InlineCacheState ic_state = UNINITIALIZED,
      ExtraICState extra_ic_state = kNoExtraICState, StubType type = NORMAL,
      CacheHolderFlag holder = kCacheOnReceiver);

  static inline Flags ComputeMonomorphicFlags(
      Kind kind, ExtraICState extra_ic_state = kNoExtraICState,
      CacheHolderFlag holder = kCacheOnReceiver, StubType type = NORMAL);

  static inline Flags ComputeHandlerFlags(
      Kind handler_kind, StubType type = NORMAL,
      CacheHolderFlag holder = kCacheOnReceiver);

  static inline InlineCacheState ExtractICStateFromFlags(Flags flags);
  static inline StubType ExtractTypeFromFlags(Flags flags);
  static inline CacheHolderFlag ExtractCacheHolderFromFlags(Flags flags);
  static inline Kind ExtractKindFromFlags(Flags flags);
  static inline ExtraICState ExtractExtraICStateFromFlags(Flags flags);

  static inline Flags RemoveTypeFromFlags(Flags flags);
  static inline Flags RemoveTypeAndHolderFromFlags(Flags flags);

  // Convert a target address into a code object.
  static inline Code* GetCodeFromTargetAddress(Address address);

  // Convert an entry address into an object.
  static inline Object* GetObjectFromEntryAddress(Address location_of_address);

  // Returns the address of the first instruction.
  inline byte* instruction_start();

  // Returns the address right after the last instruction.
  inline byte* instruction_end();

  // Returns the size of the instructions, padding, and relocation information.
  inline int body_size();

  // Returns the address of the first relocation info (read backwards!).
  inline byte* relocation_start();

  // Code entry point.
  inline byte* entry();

  // Returns true if pc is inside this object's instructions.
  inline bool contains(byte* pc);

  // Relocate the code by delta bytes. Called to signal that this code
  // object has been moved by delta bytes.
  void Relocate(intptr_t delta);

  // Migrate code described by desc.
  void CopyFrom(const CodeDesc& desc);

  // Returns the object size for a given body (used for allocation).
  static int SizeFor(int body_size) {
    DCHECK_SIZE_TAG_ALIGNED(body_size);
    return RoundUp(kHeaderSize + body_size, kCodeAlignment);
  }

  // Calculate the size of the code object to report for log events. This takes
  // the layout of the code object into account.
  inline int ExecutableSize();

  // Locating source position.
  int SourcePosition(Address pc);
  int SourceStatementPosition(Address pc);

  DECLARE_CAST(Code)

  // Dispatched behavior.
  inline int CodeSize();
  inline void CodeIterateBody(ObjectVisitor* v);

  template<typename StaticVisitor>
  inline void CodeIterateBody(Heap* heap);

  DECLARE_PRINTER(Code)
  DECLARE_VERIFIER(Code)

  void ClearInlineCaches();
  void ClearInlineCaches(Kind kind);

  BailoutId TranslatePcOffsetToAstId(uint32_t pc_offset);
  uint32_t TranslateAstIdToPcOffset(BailoutId ast_id);

#define DECLARE_CODE_AGE_ENUM(X) k##X##CodeAge,
  enum Age {
    kToBeExecutedOnceCodeAge = -3,
    kNotExecutedCodeAge = -2,
    kExecutedOnceCodeAge = -1,
    kNoAgeCodeAge = 0,
    CODE_AGE_LIST(DECLARE_CODE_AGE_ENUM)
    kAfterLastCodeAge,
    kFirstCodeAge = kToBeExecutedOnceCodeAge,
    kLastCodeAge = kAfterLastCodeAge - 1,
    kCodeAgeCount = kAfterLastCodeAge - kFirstCodeAge - 1,
    kIsOldCodeAge = kSexagenarianCodeAge,
    kPreAgedCodeAge = kIsOldCodeAge - 1
  };
#undef DECLARE_CODE_AGE_ENUM

  // Code aging.  Indicates how many full GCs this code has survived without
  // being entered through the prologue.  Used to determine when it is
  // relatively safe to flush this code object and replace it with the lazy
  // compilation stub.
  static void MakeCodeAgeSequenceYoung(byte* sequence, Isolate* isolate);
  static void MarkCodeAsExecuted(byte* sequence, Isolate* isolate);
  void MakeYoung(Isolate* isolate);
  void MarkToBeExecutedOnce(Isolate* isolate);
  void MakeOlder(MarkingParity);
  static bool IsYoungSequence(Isolate* isolate, byte* sequence);
  bool IsOld();
  Age GetAge();
  static inline Code* GetPreAgedCodeAgeStub(Isolate* isolate) {
    return GetCodeAgeStub(isolate, kNotExecutedCodeAge, NO_MARKING_PARITY);
  }

  void PrintDeoptLocation(FILE* out, Address pc);
  bool CanDeoptAt(Address pc);

#ifdef VERIFY_HEAP
  void VerifyEmbeddedObjectsDependency();
#endif

#ifdef DEBUG
  enum VerifyMode { kNoContextSpecificPointers, kNoContextRetainingPointers };
  void VerifyEmbeddedObjects(VerifyMode mode = kNoContextRetainingPointers);
  static void VerifyRecompiledCode(Code* old_code, Code* new_code);
#endif  // DEBUG

  inline bool CanContainWeakObjects();

  inline bool IsWeakObject(Object* object);

  static inline bool IsWeakObjectInOptimizedCode(Object* object);

  static Handle<WeakCell> WeakCellFor(Handle<Code> code);
  WeakCell* CachedWeakCell();

  // Max loop nesting marker used to postpose OSR. We don't take loop
  // nesting that is deeper than 5 levels into account.
  static const int kMaxLoopNestingMarker = 6;

  static const int kConstantPoolSize =
      FLAG_enable_embedded_constant_pool ? kIntSize : 0;

  // Layout description.
  static const int kRelocationInfoOffset = HeapObject::kHeaderSize;
  static const int kHandlerTableOffset = kRelocationInfoOffset + kPointerSize;
  static const int kDeoptimizationDataOffset =
      kHandlerTableOffset + kPointerSize;
  // For FUNCTION kind, we store the type feedback info here.
  static const int kTypeFeedbackInfoOffset =
      kDeoptimizationDataOffset + kPointerSize;
  static const int kNextCodeLinkOffset = kTypeFeedbackInfoOffset + kPointerSize;
  static const int kGCMetadataOffset = kNextCodeLinkOffset + kPointerSize;
  static const int kInstructionSizeOffset = kGCMetadataOffset + kPointerSize;
  static const int kICAgeOffset = kInstructionSizeOffset + kIntSize;
  static const int kFlagsOffset = kICAgeOffset + kIntSize;
  static const int kKindSpecificFlags1Offset = kFlagsOffset + kIntSize;
  static const int kKindSpecificFlags2Offset =
      kKindSpecificFlags1Offset + kIntSize;
  // Note: We might be able to squeeze this into the flags above.
  static const int kPrologueOffset = kKindSpecificFlags2Offset + kIntSize;
  static const int kConstantPoolOffset = kPrologueOffset + kIntSize;
  static const int kHeaderPaddingStart =
      kConstantPoolOffset + kConstantPoolSize;

  // Add padding to align the instruction start following right after
  // the Code object header.
  static const int kHeaderSize =
      (kHeaderPaddingStart + kCodeAlignmentMask) & ~kCodeAlignmentMask;

  // Byte offsets within kKindSpecificFlags1Offset.
  static const int kFullCodeFlags = kKindSpecificFlags1Offset;
  class FullCodeFlagsHasDeoptimizationSupportField:
      public BitField<bool, 0, 1> {};  // NOLINT
  class FullCodeFlagsHasDebugBreakSlotsField: public BitField<bool, 1, 1> {};
  class FullCodeFlagsHasRelocInfoForSerialization
      : public BitField<bool, 2, 1> {};
  // Bit 3 in this bitfield is unused.
  class ProfilerTicksField : public BitField<int, 4, 28> {};

  // Flags layout.  BitField<type, shift, size>.
  class ICStateField : public BitField<InlineCacheState, 0, 4> {};
  class TypeField : public BitField<StubType, 4, 1> {};
  class CacheHolderField : public BitField<CacheHolderFlag, 5, 2> {};
  class KindField : public BitField<Kind, 7, 4> {};
  class ExtraICStateField: public BitField<ExtraICState, 11,
      PlatformSmiTagging::kSmiValueSize - 11 + 1> {};  // NOLINT

  // KindSpecificFlags1 layout (STUB and OPTIMIZED_FUNCTION)
  static const int kStackSlotsFirstBit = 0;
  static const int kStackSlotsBitCount = 24;
  static const int kHasFunctionCacheBit =
      kStackSlotsFirstBit + kStackSlotsBitCount;
  static const int kMarkedForDeoptimizationBit = kHasFunctionCacheBit + 1;
  static const int kIsTurbofannedBit = kMarkedForDeoptimizationBit + 1;
  static const int kCanHaveWeakObjects = kIsTurbofannedBit + 1;

  STATIC_ASSERT(kStackSlotsFirstBit + kStackSlotsBitCount <= 32);
  STATIC_ASSERT(kCanHaveWeakObjects + 1 <= 32);

  class StackSlotsField: public BitField<int,
      kStackSlotsFirstBit, kStackSlotsBitCount> {};  // NOLINT
  class HasFunctionCacheField : public BitField<bool, kHasFunctionCacheBit, 1> {
  };  // NOLINT
  class MarkedForDeoptimizationField
      : public BitField<bool, kMarkedForDeoptimizationBit, 1> {};   // NOLINT
  class IsTurbofannedField : public BitField<bool, kIsTurbofannedBit, 1> {
  };  // NOLINT
  class CanHaveWeakObjectsField
      : public BitField<bool, kCanHaveWeakObjects, 1> {};  // NOLINT

  // KindSpecificFlags2 layout (ALL)
  static const int kIsCrankshaftedBit = 0;
  class IsCrankshaftedField: public BitField<bool,
      kIsCrankshaftedBit, 1> {};  // NOLINT

  // KindSpecificFlags2 layout (STUB and OPTIMIZED_FUNCTION)
  static const int kSafepointTableOffsetFirstBit = kIsCrankshaftedBit + 1;
  static const int kSafepointTableOffsetBitCount = 30;

  STATIC_ASSERT(kSafepointTableOffsetFirstBit +
                kSafepointTableOffsetBitCount <= 32);
  STATIC_ASSERT(1 + kSafepointTableOffsetBitCount <= 32);

  class SafepointTableOffsetField: public BitField<int,
      kSafepointTableOffsetFirstBit,
      kSafepointTableOffsetBitCount> {};  // NOLINT

  // KindSpecificFlags2 layout (FUNCTION)
  class BackEdgeTableOffsetField: public BitField<int,
      kIsCrankshaftedBit + 1, 27> {};  // NOLINT
  class AllowOSRAtLoopNestingLevelField: public BitField<int,
      kIsCrankshaftedBit + 1 + 27, 4> {};  // NOLINT
  STATIC_ASSERT(AllowOSRAtLoopNestingLevelField::kMax >= kMaxLoopNestingMarker);

  static const int kArgumentsBits = 16;
  static const int kMaxArguments = (1 << kArgumentsBits) - 1;

  // This constant should be encodable in an ARM instruction.
  static const int kFlagsNotUsedInLookup =
      TypeField::kMask | CacheHolderField::kMask;

 private:
  friend class RelocIterator;
  friend class Deoptimizer;  // For FindCodeAgeSequence.

  void ClearInlineCaches(Kind* kind);

  // Code aging
  byte* FindCodeAgeSequence();
  static void GetCodeAgeAndParity(Code* code, Age* age,
                                  MarkingParity* parity);
  static void GetCodeAgeAndParity(Isolate* isolate, byte* sequence, Age* age,
                                  MarkingParity* parity);
  static Code* GetCodeAgeStub(Isolate* isolate, Age age, MarkingParity parity);

  // Code aging -- platform-specific
  static void PatchPlatformCodeAge(Isolate* isolate,
                                   byte* sequence, Age age,
                                   MarkingParity parity);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Code);
};


// This class describes the layout of dependent codes array of a map. The
// array is partitioned into several groups of dependent codes. Each group
// contains codes with the same dependency on the map. The array has the
// following layout for n dependency groups:
//
// +----+----+-----+----+---------+----------+-----+---------+-----------+
// | C1 | C2 | ... | Cn | group 1 |  group 2 | ... | group n | undefined |
// +----+----+-----+----+---------+----------+-----+---------+-----------+
//
// The first n elements are Smis, each of them specifies the number of codes
// in the corresponding group. The subsequent elements contain grouped code
// objects in weak cells. The suffix of the array can be filled with the
// undefined value if the number of codes is less than the length of the
// array. The order of the code objects within a group is not preserved.
//
// All code indexes used in the class are counted starting from the first
// code object of the first group. In other words, code index 0 corresponds
// to array index n = kCodesStartIndex.

class DependentCode: public FixedArray {
 public:
  enum DependencyGroup {
    // Group of code that weakly embed this map and depend on being
    // deoptimized when the map is garbage collected.
    kWeakCodeGroup,
    // Group of code that embed a transition to this map, and depend on being
    // deoptimized when the transition is replaced by a new version.
    kTransitionGroup,
    // Group of code that omit run-time prototype checks for prototypes
    // described by this map. The group is deoptimized whenever an object
    // described by this map changes shape (and transitions to a new map),
    // possibly invalidating the assumptions embedded in the code.
    kPrototypeCheckGroup,
    // Group of code that depends on global property values in property cells
    // not being changed.
    kPropertyCellChangedGroup,
    // Group of code that omit run-time type checks for the field(s) introduced
    // by this map.
    kFieldTypeGroup,
    // Group of code that omit run-time type checks for initial maps of
    // constructors.
    kInitialMapChangedGroup,
    // Group of code that depends on tenuring information in AllocationSites
    // not being changed.
    kAllocationSiteTenuringChangedGroup,
    // Group of code that depends on element transition information in
    // AllocationSites not being changed.
    kAllocationSiteTransitionChangedGroup
  };

  static const int kGroupCount = kAllocationSiteTransitionChangedGroup + 1;

  // Array for holding the index of the first code object of each group.
  // The last element stores the total number of code objects.
  class GroupStartIndexes {
   public:
    explicit GroupStartIndexes(DependentCode* entries);
    void Recompute(DependentCode* entries);
    int at(int i) { return start_indexes_[i]; }
    int number_of_entries() { return start_indexes_[kGroupCount]; }
   private:
    int start_indexes_[kGroupCount + 1];
  };

  bool Contains(DependencyGroup group, WeakCell* code_cell);

  static Handle<DependentCode> InsertCompilationDependencies(
      Handle<DependentCode> entries, DependencyGroup group,
      Handle<Foreign> info);

  static Handle<DependentCode> InsertWeakCode(Handle<DependentCode> entries,
                                              DependencyGroup group,
                                              Handle<WeakCell> code_cell);

  void UpdateToFinishedCode(DependencyGroup group, Foreign* info,
                            WeakCell* code_cell);

  void RemoveCompilationDependencies(DependentCode::DependencyGroup group,
                                     Foreign* info);

  void DeoptimizeDependentCodeGroup(Isolate* isolate,
                                    DependentCode::DependencyGroup group);

  bool MarkCodeForDeoptimization(Isolate* isolate,
                                 DependentCode::DependencyGroup group);

  // The following low-level accessors should only be used by this class
  // and the mark compact collector.
  inline int number_of_entries(DependencyGroup group);
  inline void set_number_of_entries(DependencyGroup group, int value);
  inline Object* object_at(int i);
  inline void set_object_at(int i, Object* object);
  inline void clear_at(int i);
  inline void copy(int from, int to);
  DECLARE_CAST(DependentCode)

  static const char* DependencyGroupName(DependencyGroup group);
  static void SetMarkedForDeoptimization(Code* code, DependencyGroup group);

 private:
  static Handle<DependentCode> Insert(Handle<DependentCode> entries,
                                      DependencyGroup group,
                                      Handle<Object> object);
  static Handle<DependentCode> EnsureSpace(Handle<DependentCode> entries);
  // Make a room at the end of the given group by moving out the first
  // code objects of the subsequent groups.
  inline void ExtendGroup(DependencyGroup group);
  // Compact by removing cleared weak cells and return true if there was
  // any cleared weak cell.
  bool Compact();
  static int Grow(int number_of_entries) {
    if (number_of_entries < 5) return number_of_entries + 1;
    return number_of_entries * 5 / 4;
  }
  static const int kCodesStartIndex = kGroupCount;
};


class PrototypeInfo;


// All heap objects have a Map that describes their structure.
//  A Map contains information about:
//  - Size information about the object
//  - How to iterate over an object (for garbage collection)
class Map: public HeapObject {
 public:
  // Instance size.
  // Size in bytes or kVariableSizeSentinel if instances do not have
  // a fixed size.
  inline int instance_size();
  inline void set_instance_size(int value);

  // Only to clear an unused byte, remove once byte is used.
  inline void clear_unused();

  // [inobject_properties_or_constructor_function_index]: Provides access
  // to the inobject properties in case of JSObject maps, or the constructor
  // function index in case of primitive maps.
  inline int inobject_properties_or_constructor_function_index();
  inline void set_inobject_properties_or_constructor_function_index(int value);
  // Count of properties allocated in the object (JSObject only).
  inline int GetInObjectProperties();
  inline void SetInObjectProperties(int value);
  // Index of the constructor function in the native context (primitives only),
  // or the special sentinel value to indicate that there is no object wrapper
  // for the primitive (i.e. in case of null or undefined).
  static const int kNoConstructorFunctionIndex = 0;
  inline int GetConstructorFunctionIndex();
  inline void SetConstructorFunctionIndex(int value);

  // Instance type.
  inline InstanceType instance_type();
  inline void set_instance_type(InstanceType value);

  // Tells how many unused property fields are available in the
  // instance (only used for JSObject in fast mode).
  inline int unused_property_fields();
  inline void set_unused_property_fields(int value);

  // Bit field.
  inline byte bit_field() const;
  inline void set_bit_field(byte value);

  // Bit field 2.
  inline byte bit_field2() const;
  inline void set_bit_field2(byte value);

  // Bit field 3.
  inline uint32_t bit_field3() const;
  inline void set_bit_field3(uint32_t bits);

  class EnumLengthBits:             public BitField<int,
      0, kDescriptorIndexBitCount> {};  // NOLINT
  class NumberOfOwnDescriptorsBits: public BitField<int,
      kDescriptorIndexBitCount, kDescriptorIndexBitCount> {};  // NOLINT
  STATIC_ASSERT(kDescriptorIndexBitCount + kDescriptorIndexBitCount == 20);
  class DictionaryMap : public BitField<bool, 20, 1> {};
  class OwnsDescriptors : public BitField<bool, 21, 1> {};
  class IsHiddenPrototype : public BitField<bool, 22, 1> {};
  class Deprecated : public BitField<bool, 23, 1> {};
  class IsUnstable : public BitField<bool, 24, 1> {};
  class IsMigrationTarget : public BitField<bool, 25, 1> {};
  class IsStrong : public BitField<bool, 26, 1> {};
  // Bit 27 is free.

  // Keep this bit field at the very end for better code in
  // Builtins::kJSConstructStubGeneric stub.
  // This counter is used for in-object slack tracking and for map aging.
  // The in-object slack tracking is considered enabled when the counter is
  // in the range [kSlackTrackingCounterStart, kSlackTrackingCounterEnd].
  class Counter : public BitField<int, 28, 4> {};
  static const int kSlackTrackingCounterStart = 14;
  static const int kSlackTrackingCounterEnd = 8;
  static const int kRetainingCounterStart = kSlackTrackingCounterEnd - 1;
  static const int kRetainingCounterEnd = 0;

  // Tells whether the object in the prototype property will be used
  // for instances created from this function.  If the prototype
  // property is set to a value that is not a JSObject, the prototype
  // property will not be used to create instances of the function.
  // See ECMA-262, 13.2.2.
  inline void set_non_instance_prototype(bool value);
  inline bool has_non_instance_prototype();

  // Tells whether the instance has a [[Construct]] internal method.
  // This property is implemented according to ES6, section 7.2.4.
  inline void set_is_constructor(bool value);
  inline bool is_constructor() const;

  // Tells whether the instance with this map should be ignored by the
  // Object.getPrototypeOf() function and the __proto__ accessor.
  inline void set_is_hidden_prototype();
  inline bool is_hidden_prototype() const;

  // Records and queries whether the instance has a named interceptor.
  inline void set_has_named_interceptor();
  inline bool has_named_interceptor();

  // Records and queries whether the instance has an indexed interceptor.
  inline void set_has_indexed_interceptor();
  inline bool has_indexed_interceptor();

  // Tells whether the instance is undetectable.
  // An undetectable object is a special class of JSObject: 'typeof' operator
  // returns undefined, ToBoolean returns false. Otherwise it behaves like
  // a normal JS object.  It is useful for implementing undetectable
  // document.all in Firefox & Safari.
  // See https://bugzilla.mozilla.org/show_bug.cgi?id=248549.
  inline void set_is_undetectable();
  inline bool is_undetectable();

  // Tells whether the instance has a call-as-function handler.
  inline void set_is_observed();
  inline bool is_observed();

  // Tells whether the instance has a [[Call]] internal method.
  // This property is implemented according to ES6, section 7.2.3.
  inline void set_is_callable();
  inline bool is_callable() const;

  inline void set_is_strong();
  inline bool is_strong();
  inline void set_is_extensible(bool value);
  inline bool is_extensible();
  inline void set_is_prototype_map(bool value);
  inline bool is_prototype_map() const;

  inline void set_elements_kind(ElementsKind elements_kind);
  inline ElementsKind elements_kind();

  // Tells whether the instance has fast elements that are only Smis.
  inline bool has_fast_smi_elements();

  // Tells whether the instance has fast elements.
  inline bool has_fast_object_elements();
  inline bool has_fast_smi_or_object_elements();
  inline bool has_fast_double_elements();
  inline bool has_fast_elements();
  inline bool has_sloppy_arguments_elements();
  inline bool has_fixed_typed_array_elements();
  inline bool has_dictionary_elements();

  static bool IsValidElementsTransition(ElementsKind from_kind,
                                        ElementsKind to_kind);

  // Returns true if the current map doesn't have DICTIONARY_ELEMENTS but if a
  // map with DICTIONARY_ELEMENTS was found in the prototype chain.
  bool DictionaryElementsInPrototypeChainOnly();

  inline Map* ElementsTransitionMap();

  inline FixedArrayBase* GetInitialElements();

  // [raw_transitions]: Provides access to the transitions storage field.
  // Don't call set_raw_transitions() directly to overwrite transitions, use
  // the TransitionArray::ReplaceTransitions() wrapper instead!
  DECL_ACCESSORS(raw_transitions, Object)
  // [prototype_info]: Per-prototype metadata. Aliased with transitions
  // (which prototype maps don't have).
  DECL_ACCESSORS(prototype_info, Object)
  // PrototypeInfo is created lazily using this helper (which installs it on
  // the given prototype's map).
  static Handle<PrototypeInfo> GetOrCreatePrototypeInfo(
      Handle<JSObject> prototype, Isolate* isolate);
  static Handle<PrototypeInfo> GetOrCreatePrototypeInfo(
      Handle<Map> prototype_map, Isolate* isolate);

  // [prototype chain validity cell]: Associated with a prototype object,
  // stored in that object's map's PrototypeInfo, indicates that prototype
  // chains through this object are currently valid. The cell will be
  // invalidated and replaced when the prototype chain changes.
  static Handle<Cell> GetOrCreatePrototypeChainValidityCell(Handle<Map> map,
                                                            Isolate* isolate);
  static const int kPrototypeChainValid = 0;
  static const int kPrototypeChainInvalid = 1;

  Map* FindRootMap();
  Map* FindFieldOwner(int descriptor);

  inline int GetInObjectPropertyOffset(int index);

  int NumberOfFields();

  // TODO(ishell): candidate with JSObject::MigrateToMap().
  bool InstancesNeedRewriting(Map* target, int target_number_of_fields,
                              int target_inobject, int target_unused,
                              int* old_number_of_fields);
  // TODO(ishell): moveit!
  static Handle<Map> GeneralizeAllFieldRepresentations(Handle<Map> map);
  MUST_USE_RESULT static Handle<HeapType> GeneralizeFieldType(
      Representation rep1, Handle<HeapType> type1, Representation rep2,
      Handle<HeapType> type2, Isolate* isolate);
  static void GeneralizeFieldType(Handle<Map> map, int modify_index,
                                  Representation new_representation,
                                  Handle<HeapType> new_field_type);
  static Handle<Map> ReconfigureProperty(Handle<Map> map, int modify_index,
                                         PropertyKind new_kind,
                                         PropertyAttributes new_attributes,
                                         Representation new_representation,
                                         Handle<HeapType> new_field_type,
                                         StoreMode store_mode);
  static Handle<Map> CopyGeneralizeAllRepresentations(
      Handle<Map> map, int modify_index, StoreMode store_mode,
      PropertyKind kind, PropertyAttributes attributes, const char* reason);

  static Handle<Map> PrepareForDataProperty(Handle<Map> old_map,
                                            int descriptor_number,
                                            Handle<Object> value);

  static Handle<Map> Normalize(Handle<Map> map, PropertyNormalizationMode mode,
                               const char* reason);

  // Returns the constructor name (the name (possibly, inferred name) of the
  // function that was used to instantiate the object).
  String* constructor_name();

  // Tells whether the map is used for JSObjects in dictionary mode (ie
  // normalized objects, ie objects for which HasFastProperties returns false).
  // A map can never be used for both dictionary mode and fast mode JSObjects.
  // False by default and for HeapObjects that are not JSObjects.
  inline void set_dictionary_map(bool value);
  inline bool is_dictionary_map();

  // Tells whether the instance needs security checks when accessing its
  // properties.
  inline void set_is_access_check_needed(bool access_check_needed);
  inline bool is_access_check_needed();

  // Returns true if map has a non-empty stub code cache.
  inline bool has_code_cache();

  // [prototype]: implicit prototype object.
  DECL_ACCESSORS(prototype, Object)
  // TODO(jkummerow): make set_prototype private.
  static void SetPrototype(
      Handle<Map> map, Handle<Object> prototype,
      PrototypeOptimizationMode proto_mode = FAST_PROTOTYPE);

  // [constructor]: points back to the function responsible for this map.
  // The field overlaps with the back pointer. All maps in a transition tree
  // have the same constructor, so maps with back pointers can walk the
  // back pointer chain until they find the map holding their constructor.
  DECL_ACCESSORS(constructor_or_backpointer, Object)
  inline Object* GetConstructor() const;
  inline void SetConstructor(Object* constructor,
                             WriteBarrierMode mode = UPDATE_WRITE_BARRIER);
  // [back pointer]: points back to the parent map from which a transition
  // leads to this map. The field overlaps with the constructor (see above).
  inline Object* GetBackPointer();
  inline void SetBackPointer(Object* value,
                             WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

  // [instance descriptors]: describes the object.
  DECL_ACCESSORS(instance_descriptors, DescriptorArray)

  // [layout descriptor]: describes the object layout.
  DECL_ACCESSORS(layout_descriptor, LayoutDescriptor)
  // |layout descriptor| accessor which can be used from GC.
  inline LayoutDescriptor* layout_descriptor_gc_safe();
  inline bool HasFastPointerLayout() const;

  // |layout descriptor| accessor that is safe to call even when
  // FLAG_unbox_double_fields is disabled (in this case Map does not contain
  // |layout_descriptor| field at all).
  inline LayoutDescriptor* GetLayoutDescriptor();

  inline void UpdateDescriptors(DescriptorArray* descriptors,
                                LayoutDescriptor* layout_descriptor);
  inline void InitializeDescriptors(DescriptorArray* descriptors,
                                    LayoutDescriptor* layout_descriptor);

  // [stub cache]: contains stubs compiled for this map.
  DECL_ACCESSORS(code_cache, Object)

  // [dependent code]: list of optimized codes that weakly embed this map.
  DECL_ACCESSORS(dependent_code, DependentCode)

  // [weak cell cache]: cache that stores a weak cell pointing to this map.
  DECL_ACCESSORS(weak_cell_cache, Object)

  inline PropertyDetails GetLastDescriptorDetails();

  inline int LastAdded();

  inline int NumberOfOwnDescriptors();
  inline void SetNumberOfOwnDescriptors(int number);

  inline Cell* RetrieveDescriptorsPointer();

  inline int EnumLength();
  inline void SetEnumLength(int length);

  inline bool owns_descriptors();
  inline void set_owns_descriptors(bool owns_descriptors);
  inline void mark_unstable();
  inline bool is_stable();
  inline void set_migration_target(bool value);
  inline bool is_migration_target();
  inline void set_counter(int value);
  inline int counter();
  inline void deprecate();
  inline bool is_deprecated();
  inline bool CanBeDeprecated();
  // Returns a non-deprecated version of the input. If the input was not
  // deprecated, it is directly returned. Otherwise, the non-deprecated version
  // is found by re-transitioning from the root of the transition tree using the
  // descriptor array of the map. Returns MaybeHandle<Map>() if no updated map
  // is found.
  static MaybeHandle<Map> TryUpdate(Handle<Map> map) WARN_UNUSED_RESULT;

  // Returns a non-deprecated version of the input. This method may deprecate
  // existing maps along the way if encodings conflict. Not for use while
  // gathering type feedback. Use TryUpdate in those cases instead.
  static Handle<Map> Update(Handle<Map> map);

  static Handle<Map> CopyDropDescriptors(Handle<Map> map);
  static Handle<Map> CopyInsertDescriptor(Handle<Map> map,
                                          Descriptor* descriptor,
                                          TransitionFlag flag);

  MUST_USE_RESULT static MaybeHandle<Map> CopyWithField(
      Handle<Map> map,
      Handle<Name> name,
      Handle<HeapType> type,
      PropertyAttributes attributes,
      Representation representation,
      TransitionFlag flag);

  MUST_USE_RESULT static MaybeHandle<Map> CopyWithConstant(
      Handle<Map> map,
      Handle<Name> name,
      Handle<Object> constant,
      PropertyAttributes attributes,
      TransitionFlag flag);

  // Returns a new map with all transitions dropped from the given map and
  // the ElementsKind set.
  static Handle<Map> TransitionElementsTo(Handle<Map> map,
                                          ElementsKind to_kind);

  static Handle<Map> AsElementsKind(Handle<Map> map, ElementsKind kind);

  static Handle<Map> CopyAsElementsKind(Handle<Map> map,
                                        ElementsKind kind,
                                        TransitionFlag flag);

  static Handle<Map> CopyForObserved(Handle<Map> map);

  static Handle<Map> CopyForPreventExtensions(Handle<Map> map,
                                              PropertyAttributes attrs_to_add,
                                              Handle<Symbol> transition_marker,
                                              const char* reason);

  static Handle<Map> FixProxy(Handle<Map> map, InstanceType type, int size);


  // Maximal number of fast properties. Used to restrict the number of map
  // transitions to avoid an explosion in the number of maps for objects used as
  // dictionaries.
  inline bool TooManyFastProperties(StoreFromKeyed store_mode);
  static Handle<Map> TransitionToDataProperty(Handle<Map> map,
                                              Handle<Name> name,
                                              Handle<Object> value,
                                              PropertyAttributes attributes,
                                              StoreFromKeyed store_mode);
  static Handle<Map> TransitionToAccessorProperty(
      Handle<Map> map, Handle<Name> name, AccessorComponent component,
      Handle<Object> accessor, PropertyAttributes attributes);
  static Handle<Map> ReconfigureExistingProperty(Handle<Map> map,
                                                 int descriptor,
                                                 PropertyKind kind,
                                                 PropertyAttributes attributes);

  inline void AppendDescriptor(Descriptor* desc);

  // Returns a copy of the map, prepared for inserting into the transition
  // tree (if the |map| owns descriptors then the new one will share
  // descriptors with |map|).
  static Handle<Map> CopyForTransition(Handle<Map> map, const char* reason);

  // Returns a copy of the map, with all transitions dropped from the
  // instance descriptors.
  static Handle<Map> Copy(Handle<Map> map, const char* reason);
  static Handle<Map> Create(Isolate* isolate, int inobject_properties);

  // Returns the next free property index (only valid for FAST MODE).
  int NextFreePropertyIndex();

  // Returns the number of properties described in instance_descriptors
  // filtering out properties with the specified attributes.
  int NumberOfDescribedProperties(DescriptorFlag which = OWN_DESCRIPTORS,
                                  PropertyAttributes filter = NONE);

  DECLARE_CAST(Map)

  // Code cache operations.

  // Clears the code cache.
  inline void ClearCodeCache(Heap* heap);

  // Update code cache.
  static void UpdateCodeCache(Handle<Map> map,
                              Handle<Name> name,
                              Handle<Code> code);

  // Extend the descriptor array of the map with the list of descriptors.
  // In case of duplicates, the latest descriptor is used.
  static void AppendCallbackDescriptors(Handle<Map> map,
                                        Handle<Object> descriptors);

  static inline int SlackForArraySize(int old_size, int size_limit);

  static void EnsureDescriptorSlack(Handle<Map> map, int slack);

  // Returns the found code or undefined if absent.
  Object* FindInCodeCache(Name* name, Code::Flags flags);

  // Returns the non-negative index of the code object if it is in the
  // cache and -1 otherwise.
  int IndexInCodeCache(Object* name, Code* code);

  // Removes a code object from the code cache at the given index.
  void RemoveFromCodeCache(Name* name, Code* code, int index);

  // Computes a hash value for this map, to be used in HashTables and such.
  int Hash();

  // Returns the map that this map transitions to if its elements_kind
  // is changed to |elements_kind|, or NULL if no such map is cached yet.
  // |safe_to_add_transitions| is set to false if adding transitions is not
  // allowed.
  Map* LookupElementsTransitionMap(ElementsKind elements_kind);

  // Returns the transitioned map for this map with the most generic
  // elements_kind that's found in |candidates|, or null handle if no match is
  // found at all.
  static Handle<Map> FindTransitionedMap(Handle<Map> map,
                                         MapHandleList* candidates);

  inline bool CanTransition();

  inline bool IsPrimitiveMap();
  inline bool IsJSObjectMap();
  inline bool IsJSArrayMap();
  inline bool IsJSFunctionMap();
  inline bool IsStringMap();
  inline bool IsJSProxyMap();
  inline bool IsJSGlobalProxyMap();
  inline bool IsJSGlobalObjectMap();
  inline bool IsGlobalObjectMap();

  inline bool CanOmitMapChecks();

  static void AddDependentCode(Handle<Map> map,
                               DependentCode::DependencyGroup group,
                               Handle<Code> code);

  bool IsMapInArrayPrototypeChain();

  static Handle<WeakCell> WeakCellForMap(Handle<Map> map);

  // Dispatched behavior.
  DECLARE_PRINTER(Map)
  DECLARE_VERIFIER(Map)

#ifdef VERIFY_HEAP
  void DictionaryMapVerify();
  void VerifyOmittedMapChecks();
#endif

  inline int visitor_id();
  inline void set_visitor_id(int visitor_id);

  static Handle<Map> TransitionToPrototype(Handle<Map> map,
                                           Handle<Object> prototype,
                                           PrototypeOptimizationMode mode);

  static const int kMaxPreAllocatedPropertyFields = 255;

  // Layout description.
  static const int kInstanceSizesOffset = HeapObject::kHeaderSize;
  static const int kInstanceAttributesOffset = kInstanceSizesOffset + kIntSize;
  static const int kBitField3Offset = kInstanceAttributesOffset + kIntSize;
  static const int kPrototypeOffset = kBitField3Offset + kPointerSize;
  static const int kConstructorOrBackPointerOffset =
      kPrototypeOffset + kPointerSize;
  // When there is only one transition, it is stored directly in this field;
  // otherwise a transition array is used.
  // For prototype maps, this slot is used to store this map's PrototypeInfo
  // struct.
  static const int kTransitionsOrPrototypeInfoOffset =
      kConstructorOrBackPointerOffset + kPointerSize;
  static const int kDescriptorsOffset =
      kTransitionsOrPrototypeInfoOffset + kPointerSize;
#if V8_DOUBLE_FIELDS_UNBOXING
  static const int kLayoutDecriptorOffset = kDescriptorsOffset + kPointerSize;
  static const int kCodeCacheOffset = kLayoutDecriptorOffset + kPointerSize;
#else
  static const int kLayoutDecriptorOffset = 1;  // Must not be ever accessed.
  static const int kCodeCacheOffset = kDescriptorsOffset + kPointerSize;
#endif
  static const int kDependentCodeOffset = kCodeCacheOffset + kPointerSize;
  static const int kWeakCellCacheOffset = kDependentCodeOffset + kPointerSize;
  static const int kSize = kWeakCellCacheOffset + kPointerSize;

  // Layout of pointer fields. Heap iteration code relies on them
  // being continuously allocated.
  static const int kPointerFieldsBeginOffset = Map::kPrototypeOffset;
  static const int kPointerFieldsEndOffset = kSize;

  // Byte offsets within kInstanceSizesOffset.
  static const int kInstanceSizeOffset = kInstanceSizesOffset + 0;
  static const int kInObjectPropertiesOrConstructorFunctionIndexByte = 1;
  static const int kInObjectPropertiesOrConstructorFunctionIndexOffset =
      kInstanceSizesOffset + kInObjectPropertiesOrConstructorFunctionIndexByte;
  // Note there is one byte available for use here.
  static const int kUnusedByte = 2;
  static const int kUnusedOffset = kInstanceSizesOffset + kUnusedByte;
  static const int kVisitorIdByte = 3;
  static const int kVisitorIdOffset = kInstanceSizesOffset + kVisitorIdByte;

  // Byte offsets within kInstanceAttributesOffset attributes.
#if V8_TARGET_LITTLE_ENDIAN
  // Order instance type and bit field together such that they can be loaded
  // together as a 16-bit word with instance type in the lower 8 bits regardless
  // of endianess. Also provide endian-independent offset to that 16-bit word.
  static const int kInstanceTypeOffset = kInstanceAttributesOffset + 0;
  static const int kBitFieldOffset = kInstanceAttributesOffset + 1;
#else
  static const int kBitFieldOffset = kInstanceAttributesOffset + 0;
  static const int kInstanceTypeOffset = kInstanceAttributesOffset + 1;
#endif
  static const int kInstanceTypeAndBitFieldOffset =
      kInstanceAttributesOffset + 0;
  static const int kBitField2Offset = kInstanceAttributesOffset + 2;
  static const int kUnusedPropertyFieldsByte = 3;
  static const int kUnusedPropertyFieldsOffset = kInstanceAttributesOffset + 3;

  STATIC_ASSERT(kInstanceTypeAndBitFieldOffset ==
                Internals::kMapInstanceTypeAndBitFieldOffset);

  // Bit positions for bit field.
  static const int kHasNonInstancePrototype = 0;
  static const int kIsCallable = 1;
  static const int kHasNamedInterceptor = 2;
  static const int kHasIndexedInterceptor = 3;
  static const int kIsUndetectable = 4;
  static const int kIsObserved = 5;
  static const int kIsAccessCheckNeeded = 6;
  static const int kIsConstructor = 7;

  // Bit positions for bit field 2
  static const int kIsExtensible = 0;
  // Bit 1 is free.
  class IsPrototypeMapBits : public BitField<bool, 2, 1> {};
  class ElementsKindBits: public BitField<ElementsKind, 3, 5> {};

  // Derived values from bit field 2
  static const int8_t kMaximumBitField2FastElementValue = static_cast<int8_t>(
      (FAST_ELEMENTS + 1) << Map::ElementsKindBits::kShift) - 1;
  static const int8_t kMaximumBitField2FastSmiElementValue =
      static_cast<int8_t>((FAST_SMI_ELEMENTS + 1) <<
                          Map::ElementsKindBits::kShift) - 1;
  static const int8_t kMaximumBitField2FastHoleyElementValue =
      static_cast<int8_t>((FAST_HOLEY_ELEMENTS + 1) <<
                          Map::ElementsKindBits::kShift) - 1;
  static const int8_t kMaximumBitField2FastHoleySmiElementValue =
      static_cast<int8_t>((FAST_HOLEY_SMI_ELEMENTS + 1) <<
                          Map::ElementsKindBits::kShift) - 1;

  typedef FixedBodyDescriptor<kPointerFieldsBeginOffset,
                              kPointerFieldsEndOffset,
                              kSize> BodyDescriptor;

  // Compares this map to another to see if they describe equivalent objects.
  // If |mode| is set to CLEAR_INOBJECT_PROPERTIES, |other| is treated as if
  // it had exactly zero inobject properties.
  // The "shared" flags of both this map and |other| are ignored.
  bool EquivalentToForNormalization(Map* other, PropertyNormalizationMode mode);

  // Returns true if given field is unboxed double.
  inline bool IsUnboxedDoubleField(FieldIndex index);

#if TRACE_MAPS
  static void TraceTransition(const char* what, Map* from, Map* to, Name* name);
  static void TraceAllTransitions(Map* map);
#endif

  static inline Handle<Map> CopyInstallDescriptorsForTesting(
      Handle<Map> map, int new_descriptor, Handle<DescriptorArray> descriptors,
      Handle<LayoutDescriptor> layout_descriptor);

 private:
  static void ConnectTransition(Handle<Map> parent, Handle<Map> child,
                                Handle<Name> name, SimpleTransitionFlag flag);

  bool EquivalentToForTransition(Map* other);
  static Handle<Map> RawCopy(Handle<Map> map, int instance_size);
  static Handle<Map> ShareDescriptor(Handle<Map> map,
                                     Handle<DescriptorArray> descriptors,
                                     Descriptor* descriptor);
  static Handle<Map> CopyInstallDescriptors(
      Handle<Map> map, int new_descriptor, Handle<DescriptorArray> descriptors,
      Handle<LayoutDescriptor> layout_descriptor);
  static Handle<Map> CopyAddDescriptor(Handle<Map> map,
                                       Descriptor* descriptor,
                                       TransitionFlag flag);
  static Handle<Map> CopyReplaceDescriptors(
      Handle<Map> map, Handle<DescriptorArray> descriptors,
      Handle<LayoutDescriptor> layout_descriptor, TransitionFlag flag,
      MaybeHandle<Name> maybe_name, const char* reason,
      SimpleTransitionFlag simple_flag);

  static Handle<Map> CopyReplaceDescriptor(Handle<Map> map,
                                           Handle<DescriptorArray> descriptors,
                                           Descriptor* descriptor,
                                           int index,
                                           TransitionFlag flag);
  static MUST_USE_RESULT MaybeHandle<Map> TryReconfigureExistingProperty(
      Handle<Map> map, int descriptor, PropertyKind kind,
      PropertyAttributes attributes, const char** reason);

  static Handle<Map> CopyNormalized(Handle<Map> map,
                                    PropertyNormalizationMode mode);

  // Fires when the layout of an object with a leaf map changes.
  // This includes adding transitions to the leaf map or changing
  // the descriptor array.
  inline void NotifyLeafMapLayoutChange();

  void DeprecateTransitionTree();
  bool DeprecateTarget(PropertyKind kind, Name* key,
                       PropertyAttributes attributes,
                       DescriptorArray* new_descriptors,
                       LayoutDescriptor* new_layout_descriptor);

  Map* FindLastMatchMap(int verbatim, int length, DescriptorArray* descriptors);

  // Update field type of the given descriptor to new representation and new
  // type. The type must be prepared for storing in descriptor array:
  // it must be either a simple type or a map wrapped in a weak cell.
  void UpdateFieldType(int descriptor_number, Handle<Name> name,
                       Representation new_representation,
                       Handle<Object> new_wrapped_type);

  void PrintReconfiguration(FILE* file, int modify_index, PropertyKind kind,
                            PropertyAttributes attributes);
  void PrintGeneralization(FILE* file,
                           const char* reason,
                           int modify_index,
                           int split,
                           int descriptors,
                           bool constant_to_field,
                           Representation old_representation,
                           Representation new_representation,
                           HeapType* old_field_type,
                           HeapType* new_field_type);

  static const int kFastPropertiesSoftLimit = 12;
  static const int kMaxFastProperties = 128;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Map);
};


// An abstract superclass, a marker class really, for simple structure classes.
// It doesn't carry much functionality but allows struct classes to be
// identified in the type system.
class Struct: public HeapObject {
 public:
  inline void InitializeBody(int object_size);
  DECLARE_CAST(Struct)
};


// A simple one-element struct, useful where smis need to be boxed.
class Box : public Struct {
 public:
  // [value]: the boxed contents.
  DECL_ACCESSORS(value, Object)

  DECLARE_CAST(Box)

  // Dispatched behavior.
  DECLARE_PRINTER(Box)
  DECLARE_VERIFIER(Box)

  static const int kValueOffset = HeapObject::kHeaderSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Box);
};


// Container for metadata stored on each prototype map.
class PrototypeInfo : public Struct {
 public:
  static const int UNREGISTERED = -1;

  // [prototype_users]: WeakFixedArray containing maps using this prototype,
  // or Smi(0) if uninitialized.
  DECL_ACCESSORS(prototype_users, Object)
  // [registry_slot]: Slot in prototype's user registry where this user
  // is stored. Returns UNREGISTERED if this prototype has not been registered.
  inline int registry_slot() const;
  inline void set_registry_slot(int slot);
  // [validity_cell]: Cell containing the validity bit for prototype chains
  // going through this object, or Smi(0) if uninitialized.
  // When a prototype object changes its map, then both its own validity cell
  // and those of all "downstream" prototypes are invalidated; handlers for a
  // given receiver embed the currently valid cell for that receiver's prototype
  // during their compilation and check it on execution.
  DECL_ACCESSORS(validity_cell, Object)
  // [constructor_name]: User-friendly name of the original constructor.
  DECL_ACCESSORS(constructor_name, Object)

  DECLARE_CAST(PrototypeInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(PrototypeInfo)
  DECLARE_VERIFIER(PrototypeInfo)

  static const int kPrototypeUsersOffset = HeapObject::kHeaderSize;
  static const int kRegistrySlotOffset = kPrototypeUsersOffset + kPointerSize;
  static const int kValidityCellOffset = kRegistrySlotOffset + kPointerSize;
  static const int kConstructorNameOffset = kValidityCellOffset + kPointerSize;
  static const int kSize = kConstructorNameOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PrototypeInfo);
};


// Pair used to store both a ScopeInfo and an extension object in the extension
// slot of a block context. Needed in the rare case where a declaration block
// scope (a "varblock" as used to desugar parameter destructuring) also contains
// a sloppy direct eval. (In no other case both are needed at the same time.)
class SloppyBlockWithEvalContextExtension : public Struct {
 public:
  // [scope_info]: Scope info.
  DECL_ACCESSORS(scope_info, ScopeInfo)
  // [extension]: Extension object.
  DECL_ACCESSORS(extension, JSObject)

  DECLARE_CAST(SloppyBlockWithEvalContextExtension)

  // Dispatched behavior.
  DECLARE_PRINTER(SloppyBlockWithEvalContextExtension)
  DECLARE_VERIFIER(SloppyBlockWithEvalContextExtension)

  static const int kScopeInfoOffset = HeapObject::kHeaderSize;
  static const int kExtensionOffset = kScopeInfoOffset + kPointerSize;
  static const int kSize = kExtensionOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SloppyBlockWithEvalContextExtension);
};


// Script describes a script which has been added to the VM.
class Script: public Struct {
 public:
  // Script types.
  enum Type {
    TYPE_NATIVE = 0,
    TYPE_EXTENSION = 1,
    TYPE_NORMAL = 2
  };

  // Script compilation types.
  enum CompilationType {
    COMPILATION_TYPE_HOST = 0,
    COMPILATION_TYPE_EVAL = 1
  };

  // Script compilation state.
  enum CompilationState {
    COMPILATION_STATE_INITIAL = 0,
    COMPILATION_STATE_COMPILED = 1
  };

  // [source]: the script source.
  DECL_ACCESSORS(source, Object)

  // [name]: the script name.
  DECL_ACCESSORS(name, Object)

  // [id]: the script id.
  DECL_ACCESSORS(id, Smi)

  // [line_offset]: script line offset in resource from where it was extracted.
  DECL_ACCESSORS(line_offset, Smi)

  // [column_offset]: script column offset in resource from where it was
  // extracted.
  DECL_ACCESSORS(column_offset, Smi)

  // [context_data]: context data for the context this script was compiled in.
  DECL_ACCESSORS(context_data, Object)

  // [wrapper]: the wrapper cache.  This is either undefined or a WeakCell.
  DECL_ACCESSORS(wrapper, HeapObject)

  // [type]: the script type.
  DECL_ACCESSORS(type, Smi)

  // [line_ends]: FixedArray of line ends positions.
  DECL_ACCESSORS(line_ends, Object)

  // [eval_from_shared]: for eval scripts the shared funcion info for the
  // function from which eval was called.
  DECL_ACCESSORS(eval_from_shared, Object)

  // [eval_from_instructions_offset]: the instruction offset in the code for the
  // function from which eval was called where eval was called.
  DECL_ACCESSORS(eval_from_instructions_offset, Smi)

  // [shared_function_infos]: weak fixed array containing all shared
  // function infos created from this script.
  DECL_ACCESSORS(shared_function_infos, Object)

  // [flags]: Holds an exciting bitfield.
  DECL_ACCESSORS(flags, Smi)

  // [source_url]: sourceURL from magic comment
  DECL_ACCESSORS(source_url, Object)

  // [source_url]: sourceMappingURL magic comment
  DECL_ACCESSORS(source_mapping_url, Object)

  // [compilation_type]: how the the script was compiled. Encoded in the
  // 'flags' field.
  inline CompilationType compilation_type();
  inline void set_compilation_type(CompilationType type);

  // [compilation_state]: determines whether the script has already been
  // compiled. Encoded in the 'flags' field.
  inline CompilationState compilation_state();
  inline void set_compilation_state(CompilationState state);

  // [hide_source]: determines whether the script source can be exposed as
  // function source. Encoded in the 'flags' field.
  inline bool hide_source();
  inline void set_hide_source(bool value);

  // [origin_options]: optional attributes set by the embedder via ScriptOrigin,
  // and used by the embedder to make decisions about the script. V8 just passes
  // this through. Encoded in the 'flags' field.
  inline v8::ScriptOriginOptions origin_options();
  inline void set_origin_options(ScriptOriginOptions origin_options);

  DECLARE_CAST(Script)

  // If script source is an external string, check that the underlying
  // resource is accessible. Otherwise, always return true.
  inline bool HasValidSource();

  // Convert code position into column number.
  static int GetColumnNumber(Handle<Script> script, int code_pos);

  // Convert code position into (zero-based) line number.
  // The non-handlified version does not allocate, but may be much slower.
  static int GetLineNumber(Handle<Script> script, int code_pos);
  int GetLineNumber(int code_pos);

  static Handle<Object> GetNameOrSourceURL(Handle<Script> script);

  // Init line_ends array with code positions of line ends inside script source.
  static void InitLineEnds(Handle<Script> script);

  // Get the JS object wrapping the given script; create it if none exists.
  static Handle<JSObject> GetWrapper(Handle<Script> script);

  // Look through the list of existing shared function infos to find one
  // that matches the function literal.  Return empty handle if not found.
  MaybeHandle<SharedFunctionInfo> FindSharedFunctionInfo(FunctionLiteral* fun);

  // Iterate over all script objects on the heap.
  class Iterator {
   public:
    explicit Iterator(Isolate* isolate);
    Script* Next();

   private:
    WeakFixedArray::Iterator iterator_;
    DISALLOW_COPY_AND_ASSIGN(Iterator);
  };

  // Dispatched behavior.
  DECLARE_PRINTER(Script)
  DECLARE_VERIFIER(Script)

  static const int kSourceOffset = HeapObject::kHeaderSize;
  static const int kNameOffset = kSourceOffset + kPointerSize;
  static const int kLineOffsetOffset = kNameOffset + kPointerSize;
  static const int kColumnOffsetOffset = kLineOffsetOffset + kPointerSize;
  static const int kContextOffset = kColumnOffsetOffset + kPointerSize;
  static const int kWrapperOffset = kContextOffset + kPointerSize;
  static const int kTypeOffset = kWrapperOffset + kPointerSize;
  static const int kLineEndsOffset = kTypeOffset + kPointerSize;
  static const int kIdOffset = kLineEndsOffset + kPointerSize;
  static const int kEvalFromSharedOffset = kIdOffset + kPointerSize;
  static const int kEvalFrominstructionsOffsetOffset =
      kEvalFromSharedOffset + kPointerSize;
  static const int kSharedFunctionInfosOffset =
      kEvalFrominstructionsOffsetOffset + kPointerSize;
  static const int kFlagsOffset = kSharedFunctionInfosOffset + kPointerSize;
  static const int kSourceUrlOffset = kFlagsOffset + kPointerSize;
  static const int kSourceMappingUrlOffset = kSourceUrlOffset + kPointerSize;
  static const int kSize = kSourceMappingUrlOffset + kPointerSize;

 private:
  int GetLineNumberWithArray(int code_pos);

  // Bit positions in the flags field.
  static const int kCompilationTypeBit = 0;
  static const int kCompilationStateBit = 1;
  static const int kHideSourceBit = 2;
  static const int kOriginOptionsShift = 3;
  static const int kOriginOptionsSize = 3;
  static const int kOriginOptionsMask = ((1 << kOriginOptionsSize) - 1)
                                        << kOriginOptionsShift;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Script);
};


// List of builtin functions we want to identify to improve code
// generation.
//
// Each entry has a name of a global object property holding an object
// optionally followed by ".prototype", a name of a builtin function
// on the object (the one the id is set for), and a label.
//
// Installation of ids for the selected builtin functions is handled
// by the bootstrapper.
#define FUNCTIONS_WITH_ID_LIST(V)                   \
  V(Array.prototype, indexOf, ArrayIndexOf)         \
  V(Array.prototype, lastIndexOf, ArrayLastIndexOf) \
  V(Array.prototype, push, ArrayPush)               \
  V(Array.prototype, pop, ArrayPop)                 \
  V(Array.prototype, shift, ArrayShift)             \
  V(Function.prototype, apply, FunctionApply)       \
  V(Function.prototype, call, FunctionCall)         \
  V(String.prototype, charCodeAt, StringCharCodeAt) \
  V(String.prototype, charAt, StringCharAt)         \
  V(String, fromCharCode, StringFromCharCode)       \
  V(Math, random, MathRandom)                       \
  V(Math, floor, MathFloor)                         \
  V(Math, round, MathRound)                         \
  V(Math, ceil, MathCeil)                           \
  V(Math, abs, MathAbs)                             \
  V(Math, log, MathLog)                             \
  V(Math, exp, MathExp)                             \
  V(Math, sqrt, MathSqrt)                           \
  V(Math, pow, MathPow)                             \
  V(Math, max, MathMax)                             \
  V(Math, min, MathMin)                             \
  V(Math, cos, MathCos)                             \
  V(Math, sin, MathSin)                             \
  V(Math, tan, MathTan)                             \
  V(Math, acos, MathAcos)                           \
  V(Math, asin, MathAsin)                           \
  V(Math, atan, MathAtan)                           \
  V(Math, atan2, MathAtan2)                         \
  V(Math, imul, MathImul)                           \
  V(Math, clz32, MathClz32)                         \
  V(Math, fround, MathFround)

#define ATOMIC_FUNCTIONS_WITH_ID_LIST(V) \
  V(Atomics, load, AtomicsLoad)          \
  V(Atomics, store, AtomicsStore)

enum BuiltinFunctionId {
  kArrayCode,
#define DECLARE_FUNCTION_ID(ignored1, ignore2, name)    \
  k##name,
  FUNCTIONS_WITH_ID_LIST(DECLARE_FUNCTION_ID)
      ATOMIC_FUNCTIONS_WITH_ID_LIST(DECLARE_FUNCTION_ID)
#undef DECLARE_FUNCTION_ID
  // Fake id for a special case of Math.pow. Note, it continues the
  // list of math functions.
  kMathPowHalf
};


// Result of searching in an optimized code map of a SharedFunctionInfo. Note
// that both {code} and {literals} can be NULL to pass search result status.
struct CodeAndLiterals {
  Code* code;            // Cached optimized code.
  FixedArray* literals;  // Cached literals array.
};


// SharedFunctionInfo describes the JSFunction information that can be
// shared by multiple instances of the function.
class SharedFunctionInfo: public HeapObject {
 public:
  // [name]: Function name.
  DECL_ACCESSORS(name, Object)

  // [code]: Function code.
  DECL_ACCESSORS(code, Code)
  inline void ReplaceCode(Code* code);

  // [optimized_code_map]: Map from native context to optimized code
  // and a shared literals array or Smi(0) if none.
  DECL_ACCESSORS(optimized_code_map, Object)

  // Returns entry from optimized code map for specified context and OSR entry.
  // Note that {code == nullptr, literals == nullptr} indicates no matching
  // entry has been found, whereas {code, literals == nullptr} indicates that
  // code is context-independent.
  CodeAndLiterals SearchOptimizedCodeMap(Context* native_context,
                                         BailoutId osr_ast_id);

  // Clear optimized code map.
  void ClearOptimizedCodeMap();

  // Removes a specific optimized code object from the optimized code map.
  // In case of non-OSR the code reference is cleared from the cache entry but
  // the entry itself is left in the map in order to proceed sharing literals.
  void EvictFromOptimizedCodeMap(Code* optimized_code, const char* reason);

  // Trims the optimized code map after entries have been removed.
  void TrimOptimizedCodeMap(int shrink_by);

  // Add a new entry to the optimized code map for context-independent code.
  static void AddSharedCodeToOptimizedCodeMap(Handle<SharedFunctionInfo> shared,
                                              Handle<Code> code);

  // Add a new entry to the optimized code map for context-dependent code.
  // |code| is either a code object or an undefined value. In the latter case
  // the entry just maps |native_context, osr_ast_id| pair to |literals| array.
  static void AddToOptimizedCodeMap(Handle<SharedFunctionInfo> shared,
                                    Handle<Context> native_context,
                                    Handle<HeapObject> code,
                                    Handle<FixedArray> literals,
                                    BailoutId osr_ast_id);

  // Set up the link between shared function info and the script. The shared
  // function info is added to the list on the script.
  static void SetScript(Handle<SharedFunctionInfo> shared,
                        Handle<Object> script_object);

  // Layout description of the optimized code map.
  static const int kNextMapIndex = 0;
  static const int kSharedCodeIndex = 1;
  static const int kEntriesStart = 2;
  static const int kContextOffset = 0;
  static const int kCachedCodeOffset = 1;
  static const int kLiteralsOffset = 2;
  static const int kOsrAstIdOffset = 3;
  static const int kEntryLength = 4;
  static const int kInitialLength = kEntriesStart + kEntryLength;

  static const int kNotFound = -1;

  // [scope_info]: Scope info.
  DECL_ACCESSORS(scope_info, ScopeInfo)

  // [construct stub]: Code stub for constructing instances of this function.
  DECL_ACCESSORS(construct_stub, Code)

  // Returns if this function has been compiled to native code yet.
  inline bool is_compiled();

  // [length]: The function length - usually the number of declared parameters.
  // Use up to 2^30 parameters.
  inline int length() const;
  inline void set_length(int value);

  // [internal formal parameter count]: The declared number of parameters.
  // For subclass constructors, also includes new.target.
  // The size of function's frame is internal_formal_parameter_count + 1.
  inline int internal_formal_parameter_count() const;
  inline void set_internal_formal_parameter_count(int value);

  // Set the formal parameter count so the function code will be
  // called without using argument adaptor frames.
  inline void DontAdaptArguments();

  // [expected_nof_properties]: Expected number of properties for the function.
  inline int expected_nof_properties() const;
  inline void set_expected_nof_properties(int value);

  // [feedback_vector] - accumulates ast node feedback from full-codegen and
  // (increasingly) from crankshafted code where sufficient feedback isn't
  // available.
  DECL_ACCESSORS(feedback_vector, TypeFeedbackVector)

  // Unconditionally clear the type feedback vector (including vector ICs).
  void ClearTypeFeedbackInfo();

  // Clear the type feedback vector with a more subtle policy at GC time.
  void ClearTypeFeedbackInfoAtGCTime();

#if TRACE_MAPS
  // [unique_id] - For --trace-maps purposes, an identifier that's persistent
  // even if the GC moves this SharedFunctionInfo.
  inline int unique_id() const;
  inline void set_unique_id(int value);
#endif

  // [instance class name]: class name for instances.
  DECL_ACCESSORS(instance_class_name, Object)

  // [function data]: This field holds some additional data for function.
  // Currently it has one of:
  //  - a FunctionTemplateInfo to make benefit the API [IsApiFunction()].
  //  - a Smi identifying a builtin function [HasBuiltinFunctionId()].
  //  - a BytecodeArray for the interpreter [HasBytecodeArray()].
  // In the long run we don't want all functions to have this field but
  // we can fix that when we have a better model for storing hidden data
  // on objects.
  DECL_ACCESSORS(function_data, Object)

  inline bool IsApiFunction();
  inline FunctionTemplateInfo* get_api_func_data();
  inline bool HasBuiltinFunctionId();
  inline BuiltinFunctionId builtin_function_id();
  inline bool HasBytecodeArray();
  inline BytecodeArray* bytecode_array();

  // [script info]: Script from which the function originates.
  DECL_ACCESSORS(script, Object)

  // [num_literals]: Number of literals used by this function.
  inline int num_literals() const;
  inline void set_num_literals(int value);

  // [start_position_and_type]: Field used to store both the source code
  // position, whether or not the function is a function expression,
  // and whether or not the function is a toplevel function. The two
  // least significants bit indicates whether the function is an
  // expression and the rest contains the source code position.
  inline int start_position_and_type() const;
  inline void set_start_position_and_type(int value);

  // The function is subject to debugging if a debug info is attached.
  inline bool HasDebugInfo();
  inline DebugInfo* GetDebugInfo();

  // A function has debug code if the compiled code has debug break slots.
  inline bool HasDebugCode();

  // [debug info]: Debug information.
  DECL_ACCESSORS(debug_info, Object)

  // [inferred name]: Name inferred from variable or property
  // assignment of this function. Used to facilitate debugging and
  // profiling of JavaScript code written in OO style, where almost
  // all functions are anonymous but are assigned to object
  // properties.
  DECL_ACCESSORS(inferred_name, String)

  // The function's name if it is non-empty, otherwise the inferred name.
  String* DebugName();

  // Position of the 'function' token in the script source.
  inline int function_token_position() const;
  inline void set_function_token_position(int function_token_position);

  // Position of this function in the script source.
  inline int start_position() const;
  inline void set_start_position(int start_position);

  // End position of this function in the script source.
  inline int end_position() const;
  inline void set_end_position(int end_position);

  // Is this function a function expression in the source code.
  DECL_BOOLEAN_ACCESSORS(is_expression)

  // Is this function a top-level function (scripts, evals).
  DECL_BOOLEAN_ACCESSORS(is_toplevel)

  // Bit field containing various information collected by the compiler to
  // drive optimization.
  inline int compiler_hints() const;
  inline void set_compiler_hints(int value);

  inline int ast_node_count() const;
  inline void set_ast_node_count(int count);

  inline int profiler_ticks() const;
  inline void set_profiler_ticks(int ticks);

  // Inline cache age is used to infer whether the function survived a context
  // disposal or not. In the former case we reset the opt_count.
  inline int ic_age();
  inline void set_ic_age(int age);

  // Indicates if this function can be lazy compiled.
  // This is used to determine if we can safely flush code from a function
  // when doing GC if we expect that the function will no longer be used.
  DECL_BOOLEAN_ACCESSORS(allows_lazy_compilation)

  // Indicates if this function can be lazy compiled without a context.
  // This is used to determine if we can force compilation without reaching
  // the function through program execution but through other means (e.g. heap
  // iteration by the debugger).
  DECL_BOOLEAN_ACCESSORS(allows_lazy_compilation_without_context)

  // Indicates whether optimizations have been disabled for this
  // shared function info. If a function is repeatedly optimized or if
  // we cannot optimize the function we disable optimization to avoid
  // spending time attempting to optimize it again.
  DECL_BOOLEAN_ACCESSORS(optimization_disabled)

  // Indicates the language mode.
  inline LanguageMode language_mode();
  inline void set_language_mode(LanguageMode language_mode);

  // False if the function definitely does not allocate an arguments object.
  DECL_BOOLEAN_ACCESSORS(uses_arguments)

  // Indicates that this function uses a super property (or an eval that may
  // use a super property).
  // This is needed to set up the [[HomeObject]] on the function instance.
  DECL_BOOLEAN_ACCESSORS(needs_home_object)

  // True if the function has any duplicated parameter names.
  DECL_BOOLEAN_ACCESSORS(has_duplicate_parameters)

  // Indicates whether the function is a native function.
  // These needs special treatment in .call and .apply since
  // null passed as the receiver should not be translated to the
  // global object.
  DECL_BOOLEAN_ACCESSORS(native)

  // Indicate that this function should always be inlined in optimized code.
  DECL_BOOLEAN_ACCESSORS(force_inline)

  // Indicates that the function was created by the Function function.
  // Though it's anonymous, toString should treat it as if it had the name
  // "anonymous".  We don't set the name itself so that the system does not
  // see a binding for it.
  DECL_BOOLEAN_ACCESSORS(name_should_print_as_anonymous)

  // Indicates whether the function is a bound function created using
  // the bind function.
  DECL_BOOLEAN_ACCESSORS(bound)

  // Indicates that the function is anonymous (the name field can be set
  // through the API, which does not change this flag).
  DECL_BOOLEAN_ACCESSORS(is_anonymous)

  // Is this a function or top-level/eval code.
  DECL_BOOLEAN_ACCESSORS(is_function)

  // Indicates that code for this function cannot be compiled with Crankshaft.
  DECL_BOOLEAN_ACCESSORS(dont_crankshaft)

  // Indicates that code for this function cannot be flushed.
  DECL_BOOLEAN_ACCESSORS(dont_flush)

  // Indicates that this function is a generator.
  DECL_BOOLEAN_ACCESSORS(is_generator)

  // Indicates that this function is an arrow function.
  DECL_BOOLEAN_ACCESSORS(is_arrow)

  // Indicates that this function is a concise method.
  DECL_BOOLEAN_ACCESSORS(is_concise_method)

  // Indicates that this function is an accessor (getter or setter).
  DECL_BOOLEAN_ACCESSORS(is_accessor_function)

  // Indicates that this function is a default constructor.
  DECL_BOOLEAN_ACCESSORS(is_default_constructor)

  // Indicates that this function is an asm function.
  DECL_BOOLEAN_ACCESSORS(asm_function)

  // Indicates that the the shared function info is deserialized from cache.
  DECL_BOOLEAN_ACCESSORS(deserialized)

  // Indicates that the the shared function info has never been compiled before.
  DECL_BOOLEAN_ACCESSORS(never_compiled)

  inline FunctionKind kind();
  inline void set_kind(FunctionKind kind);

  // Indicates whether or not the code in the shared function support
  // deoptimization.
  inline bool has_deoptimization_support();

  // Enable deoptimization support through recompiled code.
  void EnableDeoptimizationSupport(Code* recompiled);

  // Disable (further) attempted optimization of all functions sharing this
  // shared function info.
  void DisableOptimization(BailoutReason reason);

  inline BailoutReason disable_optimization_reason();

  // Lookup the bailout ID and DCHECK that it exists in the non-optimized
  // code, returns whether it asserted (i.e., always true if assertions are
  // disabled).
  bool VerifyBailoutId(BailoutId id);

  // [source code]: Source code for the function.
  bool HasSourceCode() const;
  Handle<Object> GetSourceCode();

  // Number of times the function was optimized.
  inline int opt_count();
  inline void set_opt_count(int opt_count);

  // Number of times the function was deoptimized.
  inline void set_deopt_count(int value);
  inline int deopt_count();
  inline void increment_deopt_count();

  // Number of time we tried to re-enable optimization after it
  // was disabled due to high number of deoptimizations.
  inline void set_opt_reenable_tries(int value);
  inline int opt_reenable_tries();

  inline void TryReenableOptimization();

  // Stores deopt_count, opt_reenable_tries and ic_age as bit-fields.
  inline void set_counters(int value);
  inline int counters() const;

  // Stores opt_count and bailout_reason as bit-fields.
  inline void set_opt_count_and_bailout_reason(int value);
  inline int opt_count_and_bailout_reason() const;

  inline void set_disable_optimization_reason(BailoutReason reason);

  // Tells whether this function should be subject to debugging.
  inline bool IsSubjectToDebugging();

  // Whether this function is defined in native code or extensions.
  inline bool IsBuiltin();

  // Check whether or not this function is inlineable.
  bool IsInlineable();

  // Source size of this function.
  int SourceSize();

  // Calculate the instance size.
  int CalculateInstanceSize();

  // Calculate the number of in-object properties.
  int CalculateInObjectProperties();

  inline bool has_simple_parameters();

  // Initialize a SharedFunctionInfo from a parsed function literal.
  static void InitFromFunctionLiteral(Handle<SharedFunctionInfo> shared_info,
                                      FunctionLiteral* lit);

  // Dispatched behavior.
  DECLARE_PRINTER(SharedFunctionInfo)
  DECLARE_VERIFIER(SharedFunctionInfo)

  void ResetForNewContext(int new_ic_age);

  // Iterate over all shared function infos that are created from a script.
  // That excludes shared function infos created for API functions and C++
  // builtins.
  class Iterator {
   public:
    explicit Iterator(Isolate* isolate);
    SharedFunctionInfo* Next();

   private:
    bool NextScript();

    Script::Iterator script_iterator_;
    WeakFixedArray::Iterator sfi_iterator_;
    DisallowHeapAllocation no_gc_;
    DISALLOW_COPY_AND_ASSIGN(Iterator);
  };

  DECLARE_CAST(SharedFunctionInfo)

  // Constants.
  static const int kDontAdaptArgumentsSentinel = -1;

  // Layout description.
  // Pointer fields.
  static const int kNameOffset = HeapObject::kHeaderSize;
  static const int kCodeOffset = kNameOffset + kPointerSize;
  static const int kOptimizedCodeMapOffset = kCodeOffset + kPointerSize;
  static const int kScopeInfoOffset = kOptimizedCodeMapOffset + kPointerSize;
  static const int kConstructStubOffset = kScopeInfoOffset + kPointerSize;
  static const int kInstanceClassNameOffset =
      kConstructStubOffset + kPointerSize;
  static const int kFunctionDataOffset =
      kInstanceClassNameOffset + kPointerSize;
  static const int kScriptOffset = kFunctionDataOffset + kPointerSize;
  static const int kDebugInfoOffset = kScriptOffset + kPointerSize;
  static const int kInferredNameOffset = kDebugInfoOffset + kPointerSize;
  static const int kFeedbackVectorOffset =
      kInferredNameOffset + kPointerSize;
#if TRACE_MAPS
  static const int kUniqueIdOffset = kFeedbackVectorOffset + kPointerSize;
  static const int kLastPointerFieldOffset = kUniqueIdOffset;
#else
  // Just to not break the postmortrem support with conditional offsets
  static const int kUniqueIdOffset = kFeedbackVectorOffset;
  static const int kLastPointerFieldOffset = kFeedbackVectorOffset;
#endif

#if V8_HOST_ARCH_32_BIT
  // Smi fields.
  static const int kLengthOffset = kLastPointerFieldOffset + kPointerSize;
  static const int kFormalParameterCountOffset = kLengthOffset + kPointerSize;
  static const int kExpectedNofPropertiesOffset =
      kFormalParameterCountOffset + kPointerSize;
  static const int kNumLiteralsOffset =
      kExpectedNofPropertiesOffset + kPointerSize;
  static const int kStartPositionAndTypeOffset =
      kNumLiteralsOffset + kPointerSize;
  static const int kEndPositionOffset =
      kStartPositionAndTypeOffset + kPointerSize;
  static const int kFunctionTokenPositionOffset =
      kEndPositionOffset + kPointerSize;
  static const int kCompilerHintsOffset =
      kFunctionTokenPositionOffset + kPointerSize;
  static const int kOptCountAndBailoutReasonOffset =
      kCompilerHintsOffset + kPointerSize;
  static const int kCountersOffset =
      kOptCountAndBailoutReasonOffset + kPointerSize;
  static const int kAstNodeCountOffset =
      kCountersOffset + kPointerSize;
  static const int kProfilerTicksOffset =
      kAstNodeCountOffset + kPointerSize;

  // Total size.
  static const int kSize = kProfilerTicksOffset + kPointerSize;
#else
  // The only reason to use smi fields instead of int fields
  // is to allow iteration without maps decoding during
  // garbage collections.
  // To avoid wasting space on 64-bit architectures we use
  // the following trick: we group integer fields into pairs
// The least significant integer in each pair is shifted left by 1.
// By doing this we guarantee that LSB of each kPointerSize aligned
// word is not set and thus this word cannot be treated as pointer
// to HeapObject during old space traversal.
#if V8_TARGET_LITTLE_ENDIAN
  static const int kLengthOffset = kLastPointerFieldOffset + kPointerSize;
  static const int kFormalParameterCountOffset =
      kLengthOffset + kIntSize;

  static const int kExpectedNofPropertiesOffset =
      kFormalParameterCountOffset + kIntSize;
  static const int kNumLiteralsOffset =
      kExpectedNofPropertiesOffset + kIntSize;

  static const int kEndPositionOffset =
      kNumLiteralsOffset + kIntSize;
  static const int kStartPositionAndTypeOffset =
      kEndPositionOffset + kIntSize;

  static const int kFunctionTokenPositionOffset =
      kStartPositionAndTypeOffset + kIntSize;
  static const int kCompilerHintsOffset =
      kFunctionTokenPositionOffset + kIntSize;

  static const int kOptCountAndBailoutReasonOffset =
      kCompilerHintsOffset + kIntSize;
  static const int kCountersOffset =
      kOptCountAndBailoutReasonOffset + kIntSize;

  static const int kAstNodeCountOffset =
      kCountersOffset + kIntSize;
  static const int kProfilerTicksOffset =
      kAstNodeCountOffset + kIntSize;

  // Total size.
  static const int kSize = kProfilerTicksOffset + kIntSize;

#elif V8_TARGET_BIG_ENDIAN
  static const int kFormalParameterCountOffset =
      kLastPointerFieldOffset + kPointerSize;
  static const int kLengthOffset = kFormalParameterCountOffset + kIntSize;

  static const int kNumLiteralsOffset = kLengthOffset + kIntSize;
  static const int kExpectedNofPropertiesOffset = kNumLiteralsOffset + kIntSize;

  static const int kStartPositionAndTypeOffset =
      kExpectedNofPropertiesOffset + kIntSize;
  static const int kEndPositionOffset = kStartPositionAndTypeOffset + kIntSize;

  static const int kCompilerHintsOffset = kEndPositionOffset + kIntSize;
  static const int kFunctionTokenPositionOffset =
      kCompilerHintsOffset + kIntSize;

  static const int kCountersOffset = kFunctionTokenPositionOffset + kIntSize;
  static const int kOptCountAndBailoutReasonOffset = kCountersOffset + kIntSize;

  static const int kProfilerTicksOffset =
      kOptCountAndBailoutReasonOffset + kIntSize;
  static const int kAstNodeCountOffset = kProfilerTicksOffset + kIntSize;

  // Total size.
  static const int kSize = kAstNodeCountOffset + kIntSize;

#else
#error Unknown byte ordering
#endif  // Big endian
#endif  // 64-bit


  static const int kAlignedSize = POINTER_SIZE_ALIGN(kSize);

  typedef FixedBodyDescriptor<kNameOffset,
                              kLastPointerFieldOffset + kPointerSize,
                              kSize> BodyDescriptor;

  // Bit positions in start_position_and_type.
  // The source code start position is in the 30 most significant bits of
  // the start_position_and_type field.
  static const int kIsExpressionBit    = 0;
  static const int kIsTopLevelBit      = 1;
  static const int kStartPositionShift = 2;
  static const int kStartPositionMask  = ~((1 << kStartPositionShift) - 1);

  // Bit positions in compiler_hints.
  enum CompilerHints {
    kAllowLazyCompilation,
    kAllowLazyCompilationWithoutContext,
    kOptimizationDisabled,
    kNative,
    kStrictModeFunction,
    kStrongModeFunction,
    kUsesArguments,
    kNeedsHomeObject,
    kHasDuplicateParameters,
    kForceInline,
    kBoundFunction,
    kIsAnonymous,
    kNameShouldPrintAsAnonymous,
    kIsFunction,
    kDontCrankshaft,
    kDontFlush,
    kIsArrow,
    kIsGenerator,
    kIsConciseMethod,
    kIsAccessorFunction,
    kIsDefaultConstructor,
    kIsSubclassConstructor,
    kIsBaseConstructor,
    kInClassLiteral,
    kIsAsmFunction,
    kDeserialized,
    kNeverCompiled,
    kCompilerHintsCount  // Pseudo entry
  };
  // Add hints for other modes when they're added.
  STATIC_ASSERT(LANGUAGE_END == 3);

  class FunctionKindBits : public BitField<FunctionKind, kIsArrow, 8> {};

  class DeoptCountBits : public BitField<int, 0, 4> {};
  class OptReenableTriesBits : public BitField<int, 4, 18> {};
  class ICAgeBits : public BitField<int, 22, 8> {};

  class OptCountBits : public BitField<int, 0, 22> {};
  class DisabledOptimizationReasonBits : public BitField<int, 22, 8> {};

 private:
#if V8_HOST_ARCH_32_BIT
  // On 32 bit platforms, compiler hints is a smi.
  static const int kCompilerHintsSmiTagSize = kSmiTagSize;
  static const int kCompilerHintsSize = kPointerSize;
#else
  // On 64 bit platforms, compiler hints is not a smi, see comment above.
  static const int kCompilerHintsSmiTagSize = 0;
  static const int kCompilerHintsSize = kIntSize;
#endif

  STATIC_ASSERT(SharedFunctionInfo::kCompilerHintsCount <=
                SharedFunctionInfo::kCompilerHintsSize * kBitsPerByte);

 public:
  // Constants for optimizing codegen for strict mode function and
  // native tests.
  // Allows to use byte-width instructions.
  static const int kStrictModeBitWithinByte =
      (kStrictModeFunction + kCompilerHintsSmiTagSize) % kBitsPerByte;
  static const int kStrongModeBitWithinByte =
      (kStrongModeFunction + kCompilerHintsSmiTagSize) % kBitsPerByte;

  static const int kNativeBitWithinByte =
      (kNative + kCompilerHintsSmiTagSize) % kBitsPerByte;

  static const int kBoundBitWithinByte =
      (kBoundFunction + kCompilerHintsSmiTagSize) % kBitsPerByte;

#if defined(V8_TARGET_LITTLE_ENDIAN)
  static const int kStrictModeByteOffset = kCompilerHintsOffset +
      (kStrictModeFunction + kCompilerHintsSmiTagSize) / kBitsPerByte;
  static const int kStrongModeByteOffset =
      kCompilerHintsOffset +
      (kStrongModeFunction + kCompilerHintsSmiTagSize) / kBitsPerByte;
  static const int kNativeByteOffset = kCompilerHintsOffset +
      (kNative + kCompilerHintsSmiTagSize) / kBitsPerByte;
  static const int kBoundByteOffset =
      kCompilerHintsOffset +
      (kBoundFunction + kCompilerHintsSmiTagSize) / kBitsPerByte;
#elif defined(V8_TARGET_BIG_ENDIAN)
  static const int kStrictModeByteOffset = kCompilerHintsOffset +
      (kCompilerHintsSize - 1) -
      ((kStrictModeFunction + kCompilerHintsSmiTagSize) / kBitsPerByte);
  static const int kStrongModeByteOffset =
      kCompilerHintsOffset + (kCompilerHintsSize - 1) -
      ((kStrongModeFunction + kCompilerHintsSmiTagSize) / kBitsPerByte);
  static const int kNativeByteOffset = kCompilerHintsOffset +
      (kCompilerHintsSize - 1) -
      ((kNative + kCompilerHintsSmiTagSize) / kBitsPerByte);
  static const int kBoundByteOffset =
      kCompilerHintsOffset + (kCompilerHintsSize - 1) -
      ((kBoundFunction + kCompilerHintsSmiTagSize) / kBitsPerByte);
#else
#error Unknown byte ordering
#endif

 private:
  // Returns entry from optimized code map for specified context and OSR entry.
  // The result is either kNotFound, kSharedCodeIndex for context-independent
  // entry or a start index of the context-dependent entry.
  int SearchOptimizedCodeMapEntry(Context* native_context,
                                  BailoutId osr_ast_id);

  DISALLOW_IMPLICIT_CONSTRUCTORS(SharedFunctionInfo);
};


// Printing support.
struct SourceCodeOf {
  explicit SourceCodeOf(SharedFunctionInfo* v, int max = -1)
      : value(v), max_length(max) {}
  const SharedFunctionInfo* value;
  int max_length;
};


std::ostream& operator<<(std::ostream& os, const SourceCodeOf& v);


class JSGeneratorObject: public JSObject {
 public:
  // [function]: The function corresponding to this generator object.
  DECL_ACCESSORS(function, JSFunction)

  // [context]: The context of the suspended computation.
  DECL_ACCESSORS(context, Context)

  // [receiver]: The receiver of the suspended computation.
  DECL_ACCESSORS(receiver, Object)

  // [continuation]: Offset into code of continuation.
  //
  // A positive offset indicates a suspended generator.  The special
  // kGeneratorExecuting and kGeneratorClosed values indicate that a generator
  // cannot be resumed.
  inline int continuation() const;
  inline void set_continuation(int continuation);
  inline bool is_closed();
  inline bool is_executing();
  inline bool is_suspended();

  // [operand_stack]: Saved operand stack.
  DECL_ACCESSORS(operand_stack, FixedArray)

  DECLARE_CAST(JSGeneratorObject)

  // Dispatched behavior.
  DECLARE_PRINTER(JSGeneratorObject)
  DECLARE_VERIFIER(JSGeneratorObject)

  // Magic sentinel values for the continuation.
  static const int kGeneratorExecuting = -1;
  static const int kGeneratorClosed = 0;

  // Layout description.
  static const int kFunctionOffset = JSObject::kHeaderSize;
  static const int kContextOffset = kFunctionOffset + kPointerSize;
  static const int kReceiverOffset = kContextOffset + kPointerSize;
  static const int kContinuationOffset = kReceiverOffset + kPointerSize;
  static const int kOperandStackOffset = kContinuationOffset + kPointerSize;
  static const int kSize = kOperandStackOffset + kPointerSize;

  // Resume mode, for use by runtime functions.
  enum ResumeMode { NEXT, THROW };

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSGeneratorObject);
};


// Representation for module instance objects.
class JSModule: public JSObject {
 public:
  // [context]: the context holding the module's locals, or undefined if none.
  DECL_ACCESSORS(context, Object)

  // [scope_info]: Scope info.
  DECL_ACCESSORS(scope_info, ScopeInfo)

  DECLARE_CAST(JSModule)

  // Dispatched behavior.
  DECLARE_PRINTER(JSModule)
  DECLARE_VERIFIER(JSModule)

  // Layout description.
  static const int kContextOffset = JSObject::kHeaderSize;
  static const int kScopeInfoOffset = kContextOffset + kPointerSize;
  static const int kSize = kScopeInfoOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSModule);
};


// JSFunction describes JavaScript functions.
class JSFunction: public JSObject {
 public:
  // [prototype_or_initial_map]:
  DECL_ACCESSORS(prototype_or_initial_map, Object)

  // [shared]: The information about the function that
  // can be shared by instances.
  DECL_ACCESSORS(shared, SharedFunctionInfo)

  // [context]: The context for this function.
  inline Context* context();
  inline void set_context(Object* context);
  inline JSObject* global_proxy();

  // [code]: The generated code object for this function.  Executed
  // when the function is invoked, e.g. foo() or new foo(). See
  // [[Call]] and [[Construct]] description in ECMA-262, section
  // 8.6.2, page 27.
  inline Code* code();
  inline void set_code(Code* code);
  inline void set_code_no_write_barrier(Code* code);
  inline void ReplaceCode(Code* code);

  // Tells whether this function is builtin.
  inline bool IsBuiltin();

  // Tells whether this function inlines the given shared function info.
  bool Inlines(SharedFunctionInfo* candidate);

  // Tells whether this function should be subject to debugging.
  inline bool IsSubjectToDebugging();

  // Tells whether or not the function needs arguments adaption.
  inline bool NeedsArgumentsAdaption();

  // Tells whether or not this function has been optimized.
  inline bool IsOptimized();

  // Mark this function for lazy recompilation. The function will be
  // recompiled the next time it is executed.
  void MarkForOptimization();
  void AttemptConcurrentOptimization();

  // Tells whether or not the function is already marked for lazy
  // recompilation.
  inline bool IsMarkedForOptimization();
  inline bool IsMarkedForConcurrentOptimization();

  // Tells whether or not the function is on the concurrent recompilation queue.
  inline bool IsInOptimizationQueue();

  // Inobject slack tracking is the way to reclaim unused inobject space.
  //
  // The instance size is initially determined by adding some slack to
  // expected_nof_properties (to allow for a few extra properties added
  // after the constructor). There is no guarantee that the extra space
  // will not be wasted.
  //
  // Here is the algorithm to reclaim the unused inobject space:
  // - Detect the first constructor call for this JSFunction.
  //   When it happens enter the "in progress" state: initialize construction
  //   counter in the initial_map.
  // - While the tracking is in progress create objects filled with
  //   one_pointer_filler_map instead of undefined_value. This way they can be
  //   resized quickly and safely.
  // - Once enough objects have been created  compute the 'slack'
  //   (traverse the map transition tree starting from the
  //   initial_map and find the lowest value of unused_property_fields).
  // - Traverse the transition tree again and decrease the instance size
  //   of every map. Existing objects will resize automatically (they are
  //   filled with one_pointer_filler_map). All further allocations will
  //   use the adjusted instance size.
  // - SharedFunctionInfo's expected_nof_properties left unmodified since
  //   allocations made using different closures could actually create different
  //   kind of objects (see prototype inheritance pattern).
  //
  //  Important: inobject slack tracking is not attempted during the snapshot
  //  creation.

  // True if the initial_map is set and the object constructions countdown
  // counter is not zero.
  static const int kGenerousAllocationCount =
      Map::kSlackTrackingCounterStart - Map::kSlackTrackingCounterEnd + 1;
  inline bool IsInobjectSlackTrackingInProgress();

  // Starts the tracking.
  // Initializes object constructions countdown counter in the initial map.
  void StartInobjectSlackTracking();

  // Completes the tracking.
  void CompleteInobjectSlackTracking();

  // [literals_or_bindings]: Fixed array holding either
  // the materialized literals or the bindings of a bound function.
  //
  // If the function contains object, regexp or array literals, the
  // literals array prefix contains the object, regexp, and array
  // function to be used when creating these literals.  This is
  // necessary so that we do not dynamically lookup the object, regexp
  // or array functions.  Performing a dynamic lookup, we might end up
  // using the functions from a new context that we should not have
  // access to.
  //
  // On bound functions, the array is a (copy-on-write) fixed-array containing
  // the function that was bound, bound this-value and any bound
  // arguments. Bound functions never contain literals.
  DECL_ACCESSORS(literals_or_bindings, FixedArray)

  inline FixedArray* literals();
  inline void set_literals(FixedArray* literals);

  inline FixedArray* function_bindings();
  inline void set_function_bindings(FixedArray* bindings);

  // The initial map for an object created by this constructor.
  inline Map* initial_map();
  static void SetInitialMap(Handle<JSFunction> function, Handle<Map> map,
                            Handle<Object> prototype);
  inline bool has_initial_map();
  static void EnsureHasInitialMap(Handle<JSFunction> function);

  // Get and set the prototype property on a JSFunction. If the
  // function has an initial map the prototype is set on the initial
  // map. Otherwise, the prototype is put in the initial map field
  // until an initial map is needed.
  inline bool has_prototype();
  inline bool has_instance_prototype();
  inline Object* prototype();
  inline Object* instance_prototype();
  static void SetPrototype(Handle<JSFunction> function,
                           Handle<Object> value);
  static void SetInstancePrototype(Handle<JSFunction> function,
                                   Handle<Object> value);

  // After prototype is removed, it will not be created when accessed, and
  // [[Construct]] from this function will not be allowed.
  bool RemovePrototype();

  // Accessor for this function's initial map's [[class]]
  // property. This is primarily used by ECMA native functions.  This
  // method sets the class_name field of this function's initial map
  // to a given value. It creates an initial map if this function does
  // not have one. Note that this method does not copy the initial map
  // if it has one already, but simply replaces it with the new value.
  // Instances created afterwards will have a map whose [[class]] is
  // set to 'value', but there is no guarantees on instances created
  // before.
  void SetInstanceClassName(String* name);

  // Returns if this function has been compiled to native code yet.
  inline bool is_compiled();

  // Returns `false` if formal parameters include rest parameters, optional
  // parameters, or destructuring parameters.
  // TODO(caitp): make this a flag set during parsing
  inline bool has_simple_parameters();

  // [next_function_link]: Links functions into various lists, e.g. the list
  // of optimized functions hanging off the native_context. The CodeFlusher
  // uses this link to chain together flushing candidates. Treated weakly
  // by the garbage collector.
  DECL_ACCESSORS(next_function_link, Object)

  // Prints the name of the function using PrintF.
  void PrintName(FILE* out = stdout);

  DECLARE_CAST(JSFunction)

  // Iterates the objects, including code objects indirectly referenced
  // through pointers to the first instruction in the code object.
  void JSFunctionIterateBody(int object_size, ObjectVisitor* v);

  // Dispatched behavior.
  DECLARE_PRINTER(JSFunction)
  DECLARE_VERIFIER(JSFunction)

  // Returns the number of allocated literals.
  inline int NumberOfLiterals();

  // Used for flags such as --hydrogen-filter.
  bool PassesFilter(const char* raw_filter);

  // The function's name if it is configured, otherwise shared function info
  // debug name.
  static Handle<String> GetDebugName(Handle<JSFunction> function);

  // Layout descriptors. The last property (from kNonWeakFieldsEndOffset to
  // kSize) is weak and has special handling during garbage collection.
  static const int kCodeEntryOffset = JSObject::kHeaderSize;
  static const int kPrototypeOrInitialMapOffset =
      kCodeEntryOffset + kPointerSize;
  static const int kSharedFunctionInfoOffset =
      kPrototypeOrInitialMapOffset + kPointerSize;
  static const int kContextOffset = kSharedFunctionInfoOffset + kPointerSize;
  static const int kLiteralsOffset = kContextOffset + kPointerSize;
  static const int kNonWeakFieldsEndOffset = kLiteralsOffset + kPointerSize;
  static const int kNextFunctionLinkOffset = kNonWeakFieldsEndOffset;
  static const int kSize = kNextFunctionLinkOffset + kPointerSize;

  // Layout of the bound-function binding array.
  static const int kBoundFunctionIndex = 0;
  static const int kBoundThisIndex = 1;
  static const int kBoundArgumentsStartIndex = 2;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSFunction);
};


// JSGlobalProxy's prototype must be a JSGlobalObject or null,
// and the prototype is hidden. JSGlobalProxy always delegates
// property accesses to its prototype if the prototype is not null.
//
// A JSGlobalProxy can be reinitialized which will preserve its identity.
//
// Accessing a JSGlobalProxy requires security check.

class JSGlobalProxy : public JSObject {
 public:
  // [native_context]: the owner native context of this global proxy object.
  // It is null value if this object is not used by any context.
  DECL_ACCESSORS(native_context, Object)

  // [hash]: The hash code property (undefined if not initialized yet).
  DECL_ACCESSORS(hash, Object)

  DECLARE_CAST(JSGlobalProxy)

  inline bool IsDetachedFrom(GlobalObject* global) const;

  // Dispatched behavior.
  DECLARE_PRINTER(JSGlobalProxy)
  DECLARE_VERIFIER(JSGlobalProxy)

  // Layout description.
  static const int kNativeContextOffset = JSObject::kHeaderSize;
  static const int kHashOffset = kNativeContextOffset + kPointerSize;
  static const int kSize = kHashOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSGlobalProxy);
};


// Common super class for JavaScript global objects and the special
// builtins global objects.
class GlobalObject: public JSObject {
 public:
  // [builtins]: the object holding the runtime routines written in JS.
  DECL_ACCESSORS(builtins, JSBuiltinsObject)

  // [native context]: the natives corresponding to this global object.
  DECL_ACCESSORS(native_context, Context)

  // [global proxy]: the global proxy object of the context
  DECL_ACCESSORS(global_proxy, JSObject)

  DECLARE_CAST(GlobalObject)

  static void InvalidatePropertyCell(Handle<GlobalObject> object,
                                     Handle<Name> name);
  // Ensure that the global object has a cell for the given property name.
  static Handle<PropertyCell> EnsurePropertyCell(Handle<GlobalObject> global,
                                                 Handle<Name> name);

  // Layout description.
  static const int kBuiltinsOffset = JSObject::kHeaderSize;
  static const int kNativeContextOffset = kBuiltinsOffset + kPointerSize;
  static const int kGlobalProxyOffset = kNativeContextOffset + kPointerSize;
  static const int kHeaderSize = kGlobalProxyOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GlobalObject);
};


// JavaScript global object.
class JSGlobalObject: public GlobalObject {
 public:
  DECLARE_CAST(JSGlobalObject)

  inline bool IsDetached();

  // Dispatched behavior.
  DECLARE_PRINTER(JSGlobalObject)
  DECLARE_VERIFIER(JSGlobalObject)

  // Layout description.
  static const int kSize = GlobalObject::kHeaderSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSGlobalObject);
};


// Builtins global object which holds the runtime routines written in
// JavaScript.
class JSBuiltinsObject: public GlobalObject {
 public:
  DECLARE_CAST(JSBuiltinsObject)

  // Dispatched behavior.
  DECLARE_PRINTER(JSBuiltinsObject)
  DECLARE_VERIFIER(JSBuiltinsObject)

  // Layout description.
  static const int kSize = GlobalObject::kHeaderSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSBuiltinsObject);
};


// Representation for JS Wrapper objects, String, Number, Boolean, etc.
class JSValue: public JSObject {
 public:
  // [value]: the object being wrapped.
  DECL_ACCESSORS(value, Object)

  DECLARE_CAST(JSValue)

  // Dispatched behavior.
  DECLARE_PRINTER(JSValue)
  DECLARE_VERIFIER(JSValue)

  // Layout description.
  static const int kValueOffset = JSObject::kHeaderSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSValue);
};


class DateCache;

// Representation for JS date objects.
class JSDate: public JSObject {
 public:
  // If one component is NaN, all of them are, indicating a NaN time value.
  // [value]: the time value.
  DECL_ACCESSORS(value, Object)
  // [year]: caches year. Either undefined, smi, or NaN.
  DECL_ACCESSORS(year, Object)
  // [month]: caches month. Either undefined, smi, or NaN.
  DECL_ACCESSORS(month, Object)
  // [day]: caches day. Either undefined, smi, or NaN.
  DECL_ACCESSORS(day, Object)
  // [weekday]: caches day of week. Either undefined, smi, or NaN.
  DECL_ACCESSORS(weekday, Object)
  // [hour]: caches hours. Either undefined, smi, or NaN.
  DECL_ACCESSORS(hour, Object)
  // [min]: caches minutes. Either undefined, smi, or NaN.
  DECL_ACCESSORS(min, Object)
  // [sec]: caches seconds. Either undefined, smi, or NaN.
  DECL_ACCESSORS(sec, Object)
  // [cache stamp]: sample of the date cache stamp at the
  // moment when chached fields were cached.
  DECL_ACCESSORS(cache_stamp, Object)

  DECLARE_CAST(JSDate)

  // Returns the date field with the specified index.
  // See FieldIndex for the list of date fields.
  static Object* GetField(Object* date, Smi* index);

  void SetValue(Object* value, bool is_value_nan);

  // ES6 section 20.3.4.45 Date.prototype [ @@toPrimitive ]
  static MUST_USE_RESULT MaybeHandle<Object> ToPrimitive(
      Handle<JSReceiver> receiver, Handle<Object> hint);

  // Dispatched behavior.
  DECLARE_PRINTER(JSDate)
  DECLARE_VERIFIER(JSDate)

  // The order is important. It must be kept in sync with date macros
  // in macros.py.
  enum FieldIndex {
    kDateValue,
    kYear,
    kMonth,
    kDay,
    kWeekday,
    kHour,
    kMinute,
    kSecond,
    kFirstUncachedField,
    kMillisecond = kFirstUncachedField,
    kDays,
    kTimeInDay,
    kFirstUTCField,
    kYearUTC = kFirstUTCField,
    kMonthUTC,
    kDayUTC,
    kWeekdayUTC,
    kHourUTC,
    kMinuteUTC,
    kSecondUTC,
    kMillisecondUTC,
    kDaysUTC,
    kTimeInDayUTC,
    kTimezoneOffset
  };

  // Layout description.
  static const int kValueOffset = JSObject::kHeaderSize;
  static const int kYearOffset = kValueOffset + kPointerSize;
  static const int kMonthOffset = kYearOffset + kPointerSize;
  static const int kDayOffset = kMonthOffset + kPointerSize;
  static const int kWeekdayOffset = kDayOffset + kPointerSize;
  static const int kHourOffset = kWeekdayOffset  + kPointerSize;
  static const int kMinOffset = kHourOffset + kPointerSize;
  static const int kSecOffset = kMinOffset + kPointerSize;
  static const int kCacheStampOffset = kSecOffset + kPointerSize;
  static const int kSize = kCacheStampOffset + kPointerSize;

 private:
  inline Object* DoGetField(FieldIndex index);

  Object* GetUTCField(FieldIndex index, double value, DateCache* date_cache);

  // Computes and caches the cacheable fields of the date.
  inline void SetCachedFields(int64_t local_time_ms, DateCache* date_cache);


  DISALLOW_IMPLICIT_CONSTRUCTORS(JSDate);
};


// Representation of message objects used for error reporting through
// the API. The messages are formatted in JavaScript so this object is
// a real JavaScript object. The information used for formatting the
// error messages are not directly accessible from JavaScript to
// prevent leaking information to user code called during error
// formatting.
class JSMessageObject: public JSObject {
 public:
  // [type]: the type of error message.
  inline int type() const;
  inline void set_type(int value);

  // [arguments]: the arguments for formatting the error message.
  DECL_ACCESSORS(argument, Object)

  // [script]: the script from which the error message originated.
  DECL_ACCESSORS(script, Object)

  // [stack_frames]: an array of stack frames for this error object.
  DECL_ACCESSORS(stack_frames, Object)

  // [start_position]: the start position in the script for the error message.
  inline int start_position() const;
  inline void set_start_position(int value);

  // [end_position]: the end position in the script for the error message.
  inline int end_position() const;
  inline void set_end_position(int value);

  DECLARE_CAST(JSMessageObject)

  // Dispatched behavior.
  DECLARE_PRINTER(JSMessageObject)
  DECLARE_VERIFIER(JSMessageObject)

  // Layout description.
  static const int kTypeOffset = JSObject::kHeaderSize;
  static const int kArgumentsOffset = kTypeOffset + kPointerSize;
  static const int kScriptOffset = kArgumentsOffset + kPointerSize;
  static const int kStackFramesOffset = kScriptOffset + kPointerSize;
  static const int kStartPositionOffset = kStackFramesOffset + kPointerSize;
  static const int kEndPositionOffset = kStartPositionOffset + kPointerSize;
  static const int kSize = kEndPositionOffset + kPointerSize;

  typedef FixedBodyDescriptor<HeapObject::kMapOffset,
                              kStackFramesOffset + kPointerSize,
                              kSize> BodyDescriptor;
};


// Regular expressions
// The regular expression holds a single reference to a FixedArray in
// the kDataOffset field.
// The FixedArray contains the following data:
// - tag : type of regexp implementation (not compiled yet, atom or irregexp)
// - reference to the original source string
// - reference to the original flag string
// If it is an atom regexp
// - a reference to a literal string to search for
// If it is an irregexp regexp:
// - a reference to code for Latin1 inputs (bytecode or compiled), or a smi
// used for tracking the last usage (used for code flushing).
// - a reference to code for UC16 inputs (bytecode or compiled), or a smi
// used for tracking the last usage (used for code flushing)..
// - max number of registers used by irregexp implementations.
// - number of capture registers (output values) of the regexp.
class JSRegExp: public JSObject {
 public:
  // Meaning of Type:
  // NOT_COMPILED: Initial value. No data has been stored in the JSRegExp yet.
  // ATOM: A simple string to match against using an indexOf operation.
  // IRREGEXP: Compiled with Irregexp.
  // IRREGEXP_NATIVE: Compiled to native code with Irregexp.
  enum Type { NOT_COMPILED, ATOM, IRREGEXP };
  enum Flag {
    NONE = 0,
    GLOBAL = 1,
    IGNORE_CASE = 2,
    MULTILINE = 4,
    STICKY = 8,
    UNICODE_ESCAPES = 16
  };

  class Flags {
   public:
    explicit Flags(uint32_t value) : value_(value) { }
    bool is_global() { return (value_ & GLOBAL) != 0; }
    bool is_ignore_case() { return (value_ & IGNORE_CASE) != 0; }
    bool is_multiline() { return (value_ & MULTILINE) != 0; }
    bool is_sticky() { return (value_ & STICKY) != 0; }
    bool is_unicode() { return (value_ & UNICODE_ESCAPES) != 0; }
    uint32_t value() { return value_; }
   private:
    uint32_t value_;
  };

  DECL_ACCESSORS(data, Object)

  inline Type TypeTag();
  inline int CaptureCount();
  inline Flags GetFlags();
  inline String* Pattern();
  inline Object* DataAt(int index);
  // Set implementation data after the object has been prepared.
  inline void SetDataAt(int index, Object* value);

  static int code_index(bool is_latin1) {
    if (is_latin1) {
      return kIrregexpLatin1CodeIndex;
    } else {
      return kIrregexpUC16CodeIndex;
    }
  }

  static int saved_code_index(bool is_latin1) {
    if (is_latin1) {
      return kIrregexpLatin1CodeSavedIndex;
    } else {
      return kIrregexpUC16CodeSavedIndex;
    }
  }

  DECLARE_CAST(JSRegExp)

  // Dispatched behavior.
  DECLARE_VERIFIER(JSRegExp)

  static const int kDataOffset = JSObject::kHeaderSize;
  static const int kSize = kDataOffset + kPointerSize;

  // Indices in the data array.
  static const int kTagIndex = 0;
  static const int kSourceIndex = kTagIndex + 1;
  static const int kFlagsIndex = kSourceIndex + 1;
  static const int kDataIndex = kFlagsIndex + 1;
  // The data fields are used in different ways depending on the
  // value of the tag.
  // Atom regexps (literal strings).
  static const int kAtomPatternIndex = kDataIndex;

  static const int kAtomDataSize = kAtomPatternIndex + 1;

  // Irregexp compiled code or bytecode for Latin1. If compilation
  // fails, this fields hold an exception object that should be
  // thrown if the regexp is used again.
  static const int kIrregexpLatin1CodeIndex = kDataIndex;
  // Irregexp compiled code or bytecode for UC16.  If compilation
  // fails, this fields hold an exception object that should be
  // thrown if the regexp is used again.
  static const int kIrregexpUC16CodeIndex = kDataIndex + 1;

  // Saved instance of Irregexp compiled code or bytecode for Latin1 that
  // is a potential candidate for flushing.
  static const int kIrregexpLatin1CodeSavedIndex = kDataIndex + 2;
  // Saved instance of Irregexp compiled code or bytecode for UC16 that is
  // a potential candidate for flushing.
  static const int kIrregexpUC16CodeSavedIndex = kDataIndex + 3;

  // Maximal number of registers used by either Latin1 or UC16.
  // Only used to check that there is enough stack space
  static const int kIrregexpMaxRegisterCountIndex = kDataIndex + 4;
  // Number of captures in the compiled regexp.
  static const int kIrregexpCaptureCountIndex = kDataIndex + 5;

  static const int kIrregexpDataSize = kIrregexpCaptureCountIndex + 1;

  // Offsets directly into the data fixed array.
  static const int kDataTagOffset =
      FixedArray::kHeaderSize + kTagIndex * kPointerSize;
  static const int kDataOneByteCodeOffset =
      FixedArray::kHeaderSize + kIrregexpLatin1CodeIndex * kPointerSize;
  static const int kDataUC16CodeOffset =
      FixedArray::kHeaderSize + kIrregexpUC16CodeIndex * kPointerSize;
  static const int kIrregexpCaptureCountOffset =
      FixedArray::kHeaderSize + kIrregexpCaptureCountIndex * kPointerSize;

  // In-object fields.
  static const int kSourceFieldIndex = 0;
  static const int kGlobalFieldIndex = 1;
  static const int kIgnoreCaseFieldIndex = 2;
  static const int kMultilineFieldIndex = 3;
  static const int kLastIndexFieldIndex = 4;
  static const int kInObjectFieldCount = 5;

  // The uninitialized value for a regexp code object.
  static const int kUninitializedValue = -1;

  // The compilation error value for the regexp code object. The real error
  // object is in the saved code field.
  static const int kCompilationErrorValue = -2;

  // When we store the sweep generation at which we moved the code from the
  // code index to the saved code index we mask it of to be in the [0:255]
  // range.
  static const int kCodeAgeMask = 0xff;
};


class CompilationCacheShape : public BaseShape<HashTableKey*> {
 public:
  static inline bool IsMatch(HashTableKey* key, Object* value) {
    return key->IsMatch(value);
  }

  static inline uint32_t Hash(HashTableKey* key) {
    return key->Hash();
  }

  static inline uint32_t HashForObject(HashTableKey* key, Object* object) {
    return key->HashForObject(object);
  }

  static inline Handle<Object> AsHandle(Isolate* isolate, HashTableKey* key);

  static const int kPrefixSize = 0;
  static const int kEntrySize = 2;
};


// This cache is used in two different variants. For regexp caching, it simply
// maps identifying info of the regexp to the cached regexp object. Scripts and
// eval code only gets cached after a second probe for the code object. To do
// so, on first "put" only a hash identifying the source is entered into the
// cache, mapping it to a lifetime count of the hash. On each call to Age all
// such lifetimes get reduced, and removed once they reach zero. If a second put
// is called while such a hash is live in the cache, the hash gets replaced by
// an actual cache entry. Age also removes stale live entries from the cache.
// Such entries are identified by SharedFunctionInfos pointing to either the
// recompilation stub, or to "old" code. This avoids memory leaks due to
// premature caching of scripts and eval strings that are never needed later.
class CompilationCacheTable: public HashTable<CompilationCacheTable,
                                              CompilationCacheShape,
                                              HashTableKey*> {
 public:
  // Find cached value for a string key, otherwise return null.
  Handle<Object> Lookup(
      Handle<String> src, Handle<Context> context, LanguageMode language_mode);
  Handle<Object> LookupEval(
      Handle<String> src, Handle<SharedFunctionInfo> shared,
      LanguageMode language_mode, int scope_position);
  Handle<Object> LookupRegExp(Handle<String> source, JSRegExp::Flags flags);
  static Handle<CompilationCacheTable> Put(
      Handle<CompilationCacheTable> cache, Handle<String> src,
      Handle<Context> context, LanguageMode language_mode,
      Handle<Object> value);
  static Handle<CompilationCacheTable> PutEval(
      Handle<CompilationCacheTable> cache, Handle<String> src,
      Handle<SharedFunctionInfo> context, Handle<SharedFunctionInfo> value,
      int scope_position);
  static Handle<CompilationCacheTable> PutRegExp(
      Handle<CompilationCacheTable> cache, Handle<String> src,
      JSRegExp::Flags flags, Handle<FixedArray> value);
  void Remove(Object* value);
  void Age();
  static const int kHashGenerations = 10;

  DECLARE_CAST(CompilationCacheTable)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CompilationCacheTable);
};


class CodeCache: public Struct {
 public:
  DECL_ACCESSORS(default_cache, FixedArray)
  DECL_ACCESSORS(normal_type_cache, Object)

  // Add the code object to the cache.
  static void Update(
      Handle<CodeCache> cache, Handle<Name> name, Handle<Code> code);

  // Lookup code object in the cache. Returns code object if found and undefined
  // if not.
  Object* Lookup(Name* name, Code::Flags flags);

  // Get the internal index of a code object in the cache. Returns -1 if the
  // code object is not in that cache. This index can be used to later call
  // RemoveByIndex. The cache cannot be modified between a call to GetIndex and
  // RemoveByIndex.
  int GetIndex(Object* name, Code* code);

  // Remove an object from the cache with the provided internal index.
  void RemoveByIndex(Object* name, Code* code, int index);

  DECLARE_CAST(CodeCache)

  // Dispatched behavior.
  DECLARE_PRINTER(CodeCache)
  DECLARE_VERIFIER(CodeCache)

  static const int kDefaultCacheOffset = HeapObject::kHeaderSize;
  static const int kNormalTypeCacheOffset =
      kDefaultCacheOffset + kPointerSize;
  static const int kSize = kNormalTypeCacheOffset + kPointerSize;

 private:
  static void UpdateDefaultCache(
      Handle<CodeCache> code_cache, Handle<Name> name, Handle<Code> code);
  static void UpdateNormalTypeCache(
      Handle<CodeCache> code_cache, Handle<Name> name, Handle<Code> code);
  Object* LookupDefaultCache(Name* name, Code::Flags flags);
  Object* LookupNormalTypeCache(Name* name, Code::Flags flags);

  // Code cache layout of the default cache. Elements are alternating name and
  // code objects for non normal load/store/call IC's.
  static const int kCodeCacheEntrySize = 2;
  static const int kCodeCacheEntryNameOffset = 0;
  static const int kCodeCacheEntryCodeOffset = 1;

  DISALLOW_IMPLICIT_CONSTRUCTORS(CodeCache);
};


class CodeCacheHashTableShape : public BaseShape<HashTableKey*> {
 public:
  static inline bool IsMatch(HashTableKey* key, Object* value) {
    return key->IsMatch(value);
  }

  static inline uint32_t Hash(HashTableKey* key) {
    return key->Hash();
  }

  static inline uint32_t HashForObject(HashTableKey* key, Object* object) {
    return key->HashForObject(object);
  }

  static inline Handle<Object> AsHandle(Isolate* isolate, HashTableKey* key);

  static const int kPrefixSize = 0;
  static const int kEntrySize = 2;
};


class CodeCacheHashTable: public HashTable<CodeCacheHashTable,
                                           CodeCacheHashTableShape,
                                           HashTableKey*> {
 public:
  Object* Lookup(Name* name, Code::Flags flags);
  static Handle<CodeCacheHashTable> Put(
      Handle<CodeCacheHashTable> table,
      Handle<Name> name,
      Handle<Code> code);

  int GetIndex(Name* name, Code::Flags flags);
  void RemoveByIndex(int index);

  DECLARE_CAST(CodeCacheHashTable)

  // Initial size of the fixed array backing the hash table.
  static const int kInitialSize = 64;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CodeCacheHashTable);
};


class PolymorphicCodeCache: public Struct {
 public:
  DECL_ACCESSORS(cache, Object)

  static void Update(Handle<PolymorphicCodeCache> cache,
                     MapHandleList* maps,
                     Code::Flags flags,
                     Handle<Code> code);


  // Returns an undefined value if the entry is not found.
  Handle<Object> Lookup(MapHandleList* maps, Code::Flags flags);

  DECLARE_CAST(PolymorphicCodeCache)

  // Dispatched behavior.
  DECLARE_PRINTER(PolymorphicCodeCache)
  DECLARE_VERIFIER(PolymorphicCodeCache)

  static const int kCacheOffset = HeapObject::kHeaderSize;
  static const int kSize = kCacheOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PolymorphicCodeCache);
};


class PolymorphicCodeCacheHashTable
    : public HashTable<PolymorphicCodeCacheHashTable,
                       CodeCacheHashTableShape,
                       HashTableKey*> {
 public:
  Object* Lookup(MapHandleList* maps, int code_kind);

  static Handle<PolymorphicCodeCacheHashTable> Put(
      Handle<PolymorphicCodeCacheHashTable> hash_table,
      MapHandleList* maps,
      int code_kind,
      Handle<Code> code);

  DECLARE_CAST(PolymorphicCodeCacheHashTable)

  static const int kInitialSize = 64;
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PolymorphicCodeCacheHashTable);
};


class TypeFeedbackInfo: public Struct {
 public:
  inline int ic_total_count();
  inline void set_ic_total_count(int count);

  inline int ic_with_type_info_count();
  inline void change_ic_with_type_info_count(int delta);

  inline int ic_generic_count();
  inline void change_ic_generic_count(int delta);

  inline void initialize_storage();

  inline void change_own_type_change_checksum();
  inline int own_type_change_checksum();

  inline void set_inlined_type_change_checksum(int checksum);
  inline bool matches_inlined_type_change_checksum(int checksum);

  DECLARE_CAST(TypeFeedbackInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(TypeFeedbackInfo)
  DECLARE_VERIFIER(TypeFeedbackInfo)

  static const int kStorage1Offset = HeapObject::kHeaderSize;
  static const int kStorage2Offset = kStorage1Offset + kPointerSize;
  static const int kStorage3Offset = kStorage2Offset + kPointerSize;
  static const int kSize = kStorage3Offset + kPointerSize;

 private:
  static const int kTypeChangeChecksumBits = 7;

  class ICTotalCountField: public BitField<int, 0,
      kSmiValueSize - kTypeChangeChecksumBits> {};  // NOLINT
  class OwnTypeChangeChecksum: public BitField<int,
      kSmiValueSize - kTypeChangeChecksumBits,
      kTypeChangeChecksumBits> {};  // NOLINT
  class ICsWithTypeInfoCountField: public BitField<int, 0,
      kSmiValueSize - kTypeChangeChecksumBits> {};  // NOLINT
  class InlinedTypeChangeChecksum: public BitField<int,
      kSmiValueSize - kTypeChangeChecksumBits,
      kTypeChangeChecksumBits> {};  // NOLINT

  DISALLOW_IMPLICIT_CONSTRUCTORS(TypeFeedbackInfo);
};


enum AllocationSiteMode {
  DONT_TRACK_ALLOCATION_SITE,
  TRACK_ALLOCATION_SITE,
  LAST_ALLOCATION_SITE_MODE = TRACK_ALLOCATION_SITE
};


class AllocationSite: public Struct {
 public:
  static const uint32_t kMaximumArrayBytesToPretransition = 8 * 1024;
  static const double kPretenureRatio;
  static const int kPretenureMinimumCreated = 100;

  // Values for pretenure decision field.
  enum PretenureDecision {
    kUndecided = 0,
    kDontTenure = 1,
    kMaybeTenure = 2,
    kTenure = 3,
    kZombie = 4,
    kLastPretenureDecisionValue = kZombie
  };

  const char* PretenureDecisionName(PretenureDecision decision);

  DECL_ACCESSORS(transition_info, Object)
  // nested_site threads a list of sites that represent nested literals
  // walked in a particular order. So [[1, 2], 1, 2] will have one
  // nested_site, but [[1, 2], 3, [4]] will have a list of two.
  DECL_ACCESSORS(nested_site, Object)
  DECL_ACCESSORS(pretenure_data, Smi)
  DECL_ACCESSORS(pretenure_create_count, Smi)
  DECL_ACCESSORS(dependent_code, DependentCode)
  DECL_ACCESSORS(weak_next, Object)

  inline void Initialize();

  // This method is expensive, it should only be called for reporting.
  bool IsNestedSite();

  // transition_info bitfields, for constructed array transition info.
  class ElementsKindBits:       public BitField<ElementsKind, 0,  15> {};
  class UnusedBits:             public BitField<int,          15, 14> {};
  class DoNotInlineBit:         public BitField<bool,         29,  1> {};

  // Bitfields for pretenure_data
  class MementoFoundCountBits:  public BitField<int,               0, 26> {};
  class PretenureDecisionBits:  public BitField<PretenureDecision, 26, 3> {};
  class DeoptDependentCodeBit:  public BitField<bool,              29, 1> {};
  STATIC_ASSERT(PretenureDecisionBits::kMax >= kLastPretenureDecisionValue);

  // Increments the mementos found counter and returns true when the first
  // memento was found for a given allocation site.
  inline bool IncrementMementoFoundCount();

  inline void IncrementMementoCreateCount();

  PretenureFlag GetPretenureMode();

  void ResetPretenureDecision();

  inline PretenureDecision pretenure_decision();
  inline void set_pretenure_decision(PretenureDecision decision);

  inline bool deopt_dependent_code();
  inline void set_deopt_dependent_code(bool deopt);

  inline int memento_found_count();
  inline void set_memento_found_count(int count);

  inline int memento_create_count();
  inline void set_memento_create_count(int count);

  // The pretenuring decision is made during gc, and the zombie state allows
  // us to recognize when an allocation site is just being kept alive because
  // a later traversal of new space may discover AllocationMementos that point
  // to this AllocationSite.
  inline bool IsZombie();

  inline bool IsMaybeTenure();

  inline void MarkZombie();

  inline bool MakePretenureDecision(PretenureDecision current_decision,
                                    double ratio,
                                    bool maximum_size_scavenge);

  inline bool DigestPretenuringFeedback(bool maximum_size_scavenge);

  inline ElementsKind GetElementsKind();
  inline void SetElementsKind(ElementsKind kind);

  inline bool CanInlineCall();
  inline void SetDoNotInlineCall();

  inline bool SitePointsToLiteral();

  static void DigestTransitionFeedback(Handle<AllocationSite> site,
                                       ElementsKind to_kind);

  DECLARE_PRINTER(AllocationSite)
  DECLARE_VERIFIER(AllocationSite)

  DECLARE_CAST(AllocationSite)
  static inline AllocationSiteMode GetMode(
      ElementsKind boilerplate_elements_kind);
  static inline AllocationSiteMode GetMode(ElementsKind from, ElementsKind to);
  static inline bool CanTrack(InstanceType type);

  static const int kTransitionInfoOffset = HeapObject::kHeaderSize;
  static const int kNestedSiteOffset = kTransitionInfoOffset + kPointerSize;
  static const int kPretenureDataOffset = kNestedSiteOffset + kPointerSize;
  static const int kPretenureCreateCountOffset =
      kPretenureDataOffset + kPointerSize;
  static const int kDependentCodeOffset =
      kPretenureCreateCountOffset + kPointerSize;
  static const int kWeakNextOffset = kDependentCodeOffset + kPointerSize;
  static const int kSize = kWeakNextOffset + kPointerSize;

  // During mark compact we need to take special care for the dependent code
  // field.
  static const int kPointerFieldsBeginOffset = kTransitionInfoOffset;
  static const int kPointerFieldsEndOffset = kWeakNextOffset;

  // For other visitors, use the fixed body descriptor below.
  typedef FixedBodyDescriptor<HeapObject::kHeaderSize,
                              kDependentCodeOffset + kPointerSize,
                              kSize> BodyDescriptor;

 private:
  inline bool PretenuringDecisionMade();

  DISALLOW_IMPLICIT_CONSTRUCTORS(AllocationSite);
};


class AllocationMemento: public Struct {
 public:
  static const int kAllocationSiteOffset = HeapObject::kHeaderSize;
  static const int kSize = kAllocationSiteOffset + kPointerSize;

  DECL_ACCESSORS(allocation_site, Object)

  inline bool IsValid();
  inline AllocationSite* GetAllocationSite();

  DECLARE_PRINTER(AllocationMemento)
  DECLARE_VERIFIER(AllocationMemento)

  DECLARE_CAST(AllocationMemento)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AllocationMemento);
};


// Representation of a slow alias as part of a sloppy arguments objects.
// For fast aliases (if HasSloppyArgumentsElements()):
// - the parameter map contains an index into the context
// - all attributes of the element have default values
// For slow aliases (if HasDictionaryArgumentsElements()):
// - the parameter map contains no fast alias mapping (i.e. the hole)
// - this struct (in the slow backing store) contains an index into the context
// - all attributes are available as part if the property details
class AliasedArgumentsEntry: public Struct {
 public:
  inline int aliased_context_slot() const;
  inline void set_aliased_context_slot(int count);

  DECLARE_CAST(AliasedArgumentsEntry)

  // Dispatched behavior.
  DECLARE_PRINTER(AliasedArgumentsEntry)
  DECLARE_VERIFIER(AliasedArgumentsEntry)

  static const int kAliasedContextSlot = HeapObject::kHeaderSize;
  static const int kSize = kAliasedContextSlot + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AliasedArgumentsEntry);
};


enum AllowNullsFlag {ALLOW_NULLS, DISALLOW_NULLS};
enum RobustnessFlag {ROBUST_STRING_TRAVERSAL, FAST_STRING_TRAVERSAL};


class StringHasher {
 public:
  explicit inline StringHasher(int length, uint32_t seed);

  template <typename schar>
  static inline uint32_t HashSequentialString(const schar* chars,
                                              int length,
                                              uint32_t seed);

  // Reads all the data, even for long strings and computes the utf16 length.
  static uint32_t ComputeUtf8Hash(Vector<const char> chars,
                                  uint32_t seed,
                                  int* utf16_length_out);

  // Calculated hash value for a string consisting of 1 to
  // String::kMaxArrayIndexSize digits with no leading zeros (except "0").
  // value is represented decimal value.
  static uint32_t MakeArrayIndexHash(uint32_t value, int length);

  // No string is allowed to have a hash of zero.  That value is reserved
  // for internal properties.  If the hash calculation yields zero then we
  // use 27 instead.
  static const int kZeroHash = 27;

  // Reusable parts of the hashing algorithm.
  INLINE(static uint32_t AddCharacterCore(uint32_t running_hash, uint16_t c));
  INLINE(static uint32_t GetHashCore(uint32_t running_hash));
  INLINE(static uint32_t ComputeRunningHash(uint32_t running_hash,
                                            const uc16* chars, int length));
  INLINE(static uint32_t ComputeRunningHashOneByte(uint32_t running_hash,
                                                   const char* chars,
                                                   int length));

 protected:
  // Returns the value to store in the hash field of a string with
  // the given length and contents.
  uint32_t GetHashField();
  // Returns true if the hash of this string can be computed without
  // looking at the contents.
  inline bool has_trivial_hash();
  // Adds a block of characters to the hash.
  template<typename Char>
  inline void AddCharacters(const Char* chars, int len);

 private:
  // Add a character to the hash.
  inline void AddCharacter(uint16_t c);
  // Update index. Returns true if string is still an index.
  inline bool UpdateIndex(uint16_t c);

  int length_;
  uint32_t raw_running_hash_;
  uint32_t array_index_;
  bool is_array_index_;
  bool is_first_char_;
  DISALLOW_COPY_AND_ASSIGN(StringHasher);
};


class IteratingStringHasher : public StringHasher {
 public:
  static inline uint32_t Hash(String* string, uint32_t seed);
  inline void VisitOneByteString(const uint8_t* chars, int length);
  inline void VisitTwoByteString(const uint16_t* chars, int length);

 private:
  inline IteratingStringHasher(int len, uint32_t seed);
  void VisitConsString(ConsString* cons_string);
  DISALLOW_COPY_AND_ASSIGN(IteratingStringHasher);
};


// The characteristics of a string are stored in its map.  Retrieving these
// few bits of information is moderately expensive, involving two memory
// loads where the second is dependent on the first.  To improve efficiency
// the shape of the string is given its own class so that it can be retrieved
// once and used for several string operations.  A StringShape is small enough
// to be passed by value and is immutable, but be aware that flattening a
// string can potentially alter its shape.  Also be aware that a GC caused by
// something else can alter the shape of a string due to ConsString
// shortcutting.  Keeping these restrictions in mind has proven to be error-
// prone and so we no longer put StringShapes in variables unless there is a
// concrete performance benefit at that particular point in the code.
class StringShape BASE_EMBEDDED {
 public:
  inline explicit StringShape(const String* s);
  inline explicit StringShape(Map* s);
  inline explicit StringShape(InstanceType t);
  inline bool IsSequential();
  inline bool IsExternal();
  inline bool IsCons();
  inline bool IsSliced();
  inline bool IsIndirect();
  inline bool IsExternalOneByte();
  inline bool IsExternalTwoByte();
  inline bool IsSequentialOneByte();
  inline bool IsSequentialTwoByte();
  inline bool IsInternalized();
  inline StringRepresentationTag representation_tag();
  inline uint32_t encoding_tag();
  inline uint32_t full_representation_tag();
  inline uint32_t size_tag();
#ifdef DEBUG
  inline uint32_t type() { return type_; }
  inline void invalidate() { valid_ = false; }
  inline bool valid() { return valid_; }
#else
  inline void invalidate() { }
#endif

 private:
  uint32_t type_;
#ifdef DEBUG
  inline void set_valid() { valid_ = true; }
  bool valid_;
#else
  inline void set_valid() { }
#endif
};


// The Name abstract class captures anything that can be used as a property
// name, i.e., strings and symbols.  All names store a hash value.
class Name: public HeapObject {
 public:
  // Get and set the hash field of the name.
  inline uint32_t hash_field();
  inline void set_hash_field(uint32_t value);

  // Tells whether the hash code has been computed.
  inline bool HasHashCode();

  // Returns a hash value used for the property table
  inline uint32_t Hash();

  // Equality operations.
  inline bool Equals(Name* other);
  inline static bool Equals(Handle<Name> one, Handle<Name> two);

  // Conversion.
  inline bool AsArrayIndex(uint32_t* index);

  // If the name is private, it can only name own properties.
  inline bool IsPrivate();

  // If the name is a non-flat string, this method returns a flat version of the
  // string. Otherwise it'll just return the input.
  static inline Handle<Name> Flatten(Handle<Name> name,
                                     PretenureFlag pretenure = NOT_TENURED);

  // Return a string version of this name that is converted according to the
  // rules described in ES6 section 9.2.11.
  MUST_USE_RESULT static MaybeHandle<String> ToFunctionName(Handle<Name> name);

  DECLARE_CAST(Name)

  DECLARE_PRINTER(Name)
#if TRACE_MAPS
  void NameShortPrint();
  int NameShortPrint(Vector<char> str);
#endif

  // Layout description.
  static const int kHashFieldSlot = HeapObject::kHeaderSize;
#if V8_TARGET_LITTLE_ENDIAN || !V8_HOST_ARCH_64_BIT
  static const int kHashFieldOffset = kHashFieldSlot;
#else
  static const int kHashFieldOffset = kHashFieldSlot + kIntSize;
#endif
  static const int kSize = kHashFieldSlot + kPointerSize;

  // Mask constant for checking if a name has a computed hash code
  // and if it is a string that is an array index.  The least significant bit
  // indicates whether a hash code has been computed.  If the hash code has
  // been computed the 2nd bit tells whether the string can be used as an
  // array index.
  static const int kHashNotComputedMask = 1;
  static const int kIsNotArrayIndexMask = 1 << 1;
  static const int kNofHashBitFields = 2;

  // Shift constant retrieving hash code from hash field.
  static const int kHashShift = kNofHashBitFields;

  // Only these bits are relevant in the hash, since the top two are shifted
  // out.
  static const uint32_t kHashBitMask = 0xffffffffu >> kHashShift;

  // Array index strings this short can keep their index in the hash field.
  static const int kMaxCachedArrayIndexLength = 7;

  // For strings which are array indexes the hash value has the string length
  // mixed into the hash, mainly to avoid a hash value of zero which would be
  // the case for the string '0'. 24 bits are used for the array index value.
  static const int kArrayIndexValueBits = 24;
  static const int kArrayIndexLengthBits =
      kBitsPerInt - kArrayIndexValueBits - kNofHashBitFields;

  STATIC_ASSERT((kArrayIndexLengthBits > 0));

  class ArrayIndexValueBits : public BitField<unsigned int, kNofHashBitFields,
      kArrayIndexValueBits> {};  // NOLINT
  class ArrayIndexLengthBits : public BitField<unsigned int,
      kNofHashBitFields + kArrayIndexValueBits,
      kArrayIndexLengthBits> {};  // NOLINT

  // Check that kMaxCachedArrayIndexLength + 1 is a power of two so we
  // could use a mask to test if the length of string is less than or equal to
  // kMaxCachedArrayIndexLength.
  STATIC_ASSERT(IS_POWER_OF_TWO(kMaxCachedArrayIndexLength + 1));

  static const unsigned int kContainsCachedArrayIndexMask =
      (~static_cast<unsigned>(kMaxCachedArrayIndexLength)
       << ArrayIndexLengthBits::kShift) |
      kIsNotArrayIndexMask;

  // Value of empty hash field indicating that the hash is not computed.
  static const int kEmptyHashField =
      kIsNotArrayIndexMask | kHashNotComputedMask;

 protected:
  static inline bool IsHashFieldComputed(uint32_t field);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Name);
};


// ES6 symbols.
class Symbol: public Name {
 public:
  // [name]: The print name of a symbol, or undefined if none.
  DECL_ACCESSORS(name, Object)

  DECL_ACCESSORS(flags, Smi)

  // [is_private]: Whether this is a private symbol.  Private symbols can only
  // be used to designate own properties of objects.
  DECL_BOOLEAN_ACCESSORS(is_private)

  DECLARE_CAST(Symbol)

  // Dispatched behavior.
  DECLARE_PRINTER(Symbol)
  DECLARE_VERIFIER(Symbol)

  // Layout description.
  static const int kNameOffset = Name::kSize;
  static const int kFlagsOffset = kNameOffset + kPointerSize;
  static const int kSize = kFlagsOffset + kPointerSize;

  typedef FixedBodyDescriptor<kNameOffset, kFlagsOffset, kSize> BodyDescriptor;

  void SymbolShortPrint(std::ostream& os);

 private:
  static const int kPrivateBit = 0;

  const char* PrivateSymbolToName() const;

#if TRACE_MAPS
  friend class Name;  // For PrivateSymbolToName.
#endif

  DISALLOW_IMPLICIT_CONSTRUCTORS(Symbol);
};


class ConsString;

// The String abstract class captures JavaScript string values:
//
// Ecma-262:
//  4.3.16 String Value
//    A string value is a member of the type String and is a finite
//    ordered sequence of zero or more 16-bit unsigned integer values.
//
// All string values have a length field.
class String: public Name {
 public:
  enum Encoding { ONE_BYTE_ENCODING, TWO_BYTE_ENCODING };

  // Array index strings this short can keep their index in the hash field.
  static const int kMaxCachedArrayIndexLength = 7;

  // For strings which are array indexes the hash value has the string length
  // mixed into the hash, mainly to avoid a hash value of zero which would be
  // the case for the string '0'. 24 bits are used for the array index value.
  static const int kArrayIndexValueBits = 24;
  static const int kArrayIndexLengthBits =
      kBitsPerInt - kArrayIndexValueBits - kNofHashBitFields;

  STATIC_ASSERT((kArrayIndexLengthBits > 0));

  class ArrayIndexValueBits : public BitField<unsigned int, kNofHashBitFields,
      kArrayIndexValueBits> {};  // NOLINT
  class ArrayIndexLengthBits : public BitField<unsigned int,
      kNofHashBitFields + kArrayIndexValueBits,
      kArrayIndexLengthBits> {};  // NOLINT

  // Check that kMaxCachedArrayIndexLength + 1 is a power of two so we
  // could use a mask to test if the length of string is less than or equal to
  // kMaxCachedArrayIndexLength.
  STATIC_ASSERT(IS_POWER_OF_TWO(kMaxCachedArrayIndexLength + 1));

  static const unsigned int kContainsCachedArrayIndexMask =
      (~static_cast<unsigned>(kMaxCachedArrayIndexLength)
       << ArrayIndexLengthBits::kShift) |
      kIsNotArrayIndexMask;

  class SubStringRange {
   public:
    explicit inline SubStringRange(String* string, int first = 0,
                                   int length = -1);
    class iterator;
    inline iterator begin();
    inline iterator end();

   private:
    String* string_;
    int first_;
    int length_;
  };

  // Representation of the flat content of a String.
  // A non-flat string doesn't have flat content.
  // A flat string has content that's encoded as a sequence of either
  // one-byte chars or two-byte UC16.
  // Returned by String::GetFlatContent().
  class FlatContent {
   public:
    // Returns true if the string is flat and this structure contains content.
    bool IsFlat() { return state_ != NON_FLAT; }
    // Returns true if the structure contains one-byte content.
    bool IsOneByte() { return state_ == ONE_BYTE; }
    // Returns true if the structure contains two-byte content.
    bool IsTwoByte() { return state_ == TWO_BYTE; }

    // Return the one byte content of the string. Only use if IsOneByte()
    // returns true.
    Vector<const uint8_t> ToOneByteVector() {
      DCHECK_EQ(ONE_BYTE, state_);
      return Vector<const uint8_t>(onebyte_start, length_);
    }
    // Return the two-byte content of the string. Only use if IsTwoByte()
    // returns true.
    Vector<const uc16> ToUC16Vector() {
      DCHECK_EQ(TWO_BYTE, state_);
      return Vector<const uc16>(twobyte_start, length_);
    }

    uc16 Get(int i) {
      DCHECK(i < length_);
      DCHECK(state_ != NON_FLAT);
      if (state_ == ONE_BYTE) return onebyte_start[i];
      return twobyte_start[i];
    }

    bool UsesSameString(const FlatContent& other) const {
      return onebyte_start == other.onebyte_start;
    }

   private:
    enum State { NON_FLAT, ONE_BYTE, TWO_BYTE };

    // Constructors only used by String::GetFlatContent().
    explicit FlatContent(const uint8_t* start, int length)
        : onebyte_start(start), length_(length), state_(ONE_BYTE) {}
    explicit FlatContent(const uc16* start, int length)
        : twobyte_start(start), length_(length), state_(TWO_BYTE) { }
    FlatContent() : onebyte_start(NULL), length_(0), state_(NON_FLAT) { }

    union {
      const uint8_t* onebyte_start;
      const uc16* twobyte_start;
    };
    int length_;
    State state_;

    friend class String;
    friend class IterableSubString;
  };

  template <typename Char>
  INLINE(Vector<const Char> GetCharVector());

  // Get and set the length of the string.
  inline int length() const;
  inline void set_length(int value);

  // Get and set the length of the string using acquire loads and release
  // stores.
  inline int synchronized_length() const;
  inline void synchronized_set_length(int value);

  // Returns whether this string has only one-byte chars, i.e. all of them can
  // be one-byte encoded.  This might be the case even if the string is
  // two-byte.  Such strings may appear when the embedder prefers
  // two-byte external representations even for one-byte data.
  inline bool IsOneByteRepresentation() const;
  inline bool IsTwoByteRepresentation() const;

  // Cons and slices have an encoding flag that may not represent the actual
  // encoding of the underlying string.  This is taken into account here.
  // Requires: this->IsFlat()
  inline bool IsOneByteRepresentationUnderneath();
  inline bool IsTwoByteRepresentationUnderneath();

  // NOTE: this should be considered only a hint.  False negatives are
  // possible.
  inline bool HasOnlyOneByteChars();

  // Get and set individual two byte chars in the string.
  inline void Set(int index, uint16_t value);
  // Get individual two byte char in the string.  Repeated calls
  // to this method are not efficient unless the string is flat.
  INLINE(uint16_t Get(int index));

  // ES6 section 7.1.3.1 ToNumber Applied to the String Type
  static Handle<Object> ToNumber(Handle<String> subject);

  // Flattens the string.  Checks first inline to see if it is
  // necessary.  Does nothing if the string is not a cons string.
  // Flattening allocates a sequential string with the same data as
  // the given string and mutates the cons string to a degenerate
  // form, where the first component is the new sequential string and
  // the second component is the empty string.  If allocation fails,
  // this function returns a failure.  If flattening succeeds, this
  // function returns the sequential string that is now the first
  // component of the cons string.
  //
  // Degenerate cons strings are handled specially by the garbage
  // collector (see IsShortcutCandidate).

  static inline Handle<String> Flatten(Handle<String> string,
                                       PretenureFlag pretenure = NOT_TENURED);

  // Tries to return the content of a flat string as a structure holding either
  // a flat vector of char or of uc16.
  // If the string isn't flat, and therefore doesn't have flat content, the
  // returned structure will report so, and can't provide a vector of either
  // kind.
  FlatContent GetFlatContent();

  // Returns the parent of a sliced string or first part of a flat cons string.
  // Requires: StringShape(this).IsIndirect() && this->IsFlat()
  inline String* GetUnderlying();

  // String relational comparison, implemented according to ES6 section 7.2.11
  // Abstract Relational Comparison (step 5): The comparison of Strings uses a
  // simple lexicographic ordering on sequences of code unit values. There is no
  // attempt to use the more complex, semantically oriented definitions of
  // character or string equality and collating order defined in the Unicode
  // specification. Therefore String values that are canonically equal according
  // to the Unicode standard could test as unequal. In effect this algorithm
  // assumes that both Strings are already in normalized form. Also, note that
  // for strings containing supplementary characters, lexicographic ordering on
  // sequences of UTF-16 code unit values differs from that on sequences of code
  // point values.
  MUST_USE_RESULT static ComparisonResult Compare(Handle<String> x,
                                                  Handle<String> y);

  // String equality operations.
  inline bool Equals(String* other);
  inline static bool Equals(Handle<String> one, Handle<String> two);
  bool IsUtf8EqualTo(Vector<const char> str, bool allow_prefix_match = false);
  bool IsOneByteEqualTo(Vector<const uint8_t> str);
  bool IsTwoByteEqualTo(Vector<const uc16> str);

  // Return a UTF8 representation of the string.  The string is null
  // terminated but may optionally contain nulls.  Length is returned
  // in length_output if length_output is not a null pointer  The string
  // should be nearly flat, otherwise the performance of this method may
  // be very slow (quadratic in the length).  Setting robustness_flag to
  // ROBUST_STRING_TRAVERSAL invokes behaviour that is robust  This means it
  // handles unexpected data without causing assert failures and it does not
  // do any heap allocations.  This is useful when printing stack traces.
  base::SmartArrayPointer<char> ToCString(AllowNullsFlag allow_nulls,
                                          RobustnessFlag robustness_flag,
                                          int offset, int length,
                                          int* length_output = 0);
  base::SmartArrayPointer<char> ToCString(
      AllowNullsFlag allow_nulls = DISALLOW_NULLS,
      RobustnessFlag robustness_flag = FAST_STRING_TRAVERSAL,
      int* length_output = 0);

  // Return a 16 bit Unicode representation of the string.
  // The string should be nearly flat, otherwise the performance of
  // of this method may be very bad.  Setting robustness_flag to
  // ROBUST_STRING_TRAVERSAL invokes behaviour that is robust  This means it
  // handles unexpected data without causing assert failures and it does not
  // do any heap allocations.  This is useful when printing stack traces.
  base::SmartArrayPointer<uc16> ToWideCString(
      RobustnessFlag robustness_flag = FAST_STRING_TRAVERSAL);

  bool ComputeArrayIndex(uint32_t* index);

  // Externalization.
  bool MakeExternal(v8::String::ExternalStringResource* resource);
  bool MakeExternal(v8::String::ExternalOneByteStringResource* resource);

  // Conversion.
  inline bool AsArrayIndex(uint32_t* index);

  DECLARE_CAST(String)

  void PrintOn(FILE* out);

  // For use during stack traces.  Performs rudimentary sanity check.
  bool LooksValid();

  // Dispatched behavior.
  void StringShortPrint(StringStream* accumulator);
  void PrintUC16(std::ostream& os, int start = 0, int end = -1);  // NOLINT
#if defined(DEBUG) || defined(OBJECT_PRINT)
  char* ToAsciiArray();
#endif
  DECLARE_PRINTER(String)
  DECLARE_VERIFIER(String)

  inline bool IsFlat();

  // Layout description.
  static const int kLengthOffset = Name::kSize;
  static const int kSize = kLengthOffset + kPointerSize;

  // Maximum number of characters to consider when trying to convert a string
  // value into an array index.
  static const int kMaxArrayIndexSize = 10;
  STATIC_ASSERT(kMaxArrayIndexSize < (1 << kArrayIndexLengthBits));

  // Max char codes.
  static const int32_t kMaxOneByteCharCode = unibrow::Latin1::kMaxChar;
  static const uint32_t kMaxOneByteCharCodeU = unibrow::Latin1::kMaxChar;
  static const int kMaxUtf16CodeUnit = 0xffff;
  static const uint32_t kMaxUtf16CodeUnitU = kMaxUtf16CodeUnit;

  // Value of hash field containing computed hash equal to zero.
  static const int kEmptyStringHash = kIsNotArrayIndexMask;

  // Maximal string length.
  static const int kMaxLength = (1 << 28) - 16;

  // Max length for computing hash. For strings longer than this limit the
  // string length is used as the hash value.
  static const int kMaxHashCalcLength = 16383;

  // Limit for truncation in short printing.
  static const int kMaxShortPrintLength = 1024;

  // Support for regular expressions.
  const uc16* GetTwoByteData(unsigned start);

  // Helper function for flattening strings.
  template <typename sinkchar>
  static void WriteToFlat(String* source,
                          sinkchar* sink,
                          int from,
                          int to);

  // The return value may point to the first aligned word containing the first
  // non-one-byte character, rather than directly to the non-one-byte character.
  // If the return value is >= the passed length, the entire string was
  // one-byte.
  static inline int NonAsciiStart(const char* chars, int length) {
    const char* start = chars;
    const char* limit = chars + length;

    if (length >= kIntptrSize) {
      // Check unaligned bytes.
      while (!IsAligned(reinterpret_cast<intptr_t>(chars), sizeof(uintptr_t))) {
        if (static_cast<uint8_t>(*chars) > unibrow::Utf8::kMaxOneByteChar) {
          return static_cast<int>(chars - start);
        }
        ++chars;
      }
      // Check aligned words.
      DCHECK(unibrow::Utf8::kMaxOneByteChar == 0x7F);
      const uintptr_t non_one_byte_mask = kUintptrAllBitsSet / 0xFF * 0x80;
      while (chars + sizeof(uintptr_t) <= limit) {
        if (*reinterpret_cast<const uintptr_t*>(chars) & non_one_byte_mask) {
          return static_cast<int>(chars - start);
        }
        chars += sizeof(uintptr_t);
      }
    }
    // Check remaining unaligned bytes.
    while (chars < limit) {
      if (static_cast<uint8_t>(*chars) > unibrow::Utf8::kMaxOneByteChar) {
        return static_cast<int>(chars - start);
      }
      ++chars;
    }

    return static_cast<int>(chars - start);
  }

  static inline bool IsAscii(const char* chars, int length) {
    return NonAsciiStart(chars, length) >= length;
  }

  static inline bool IsAscii(const uint8_t* chars, int length) {
    return
        NonAsciiStart(reinterpret_cast<const char*>(chars), length) >= length;
  }

  static inline int NonOneByteStart(const uc16* chars, int length) {
    const uc16* limit = chars + length;
    const uc16* start = chars;
    while (chars < limit) {
      if (*chars > kMaxOneByteCharCodeU) return static_cast<int>(chars - start);
      ++chars;
    }
    return static_cast<int>(chars - start);
  }

  static inline bool IsOneByte(const uc16* chars, int length) {
    return NonOneByteStart(chars, length) >= length;
  }

  template<class Visitor>
  static inline ConsString* VisitFlat(Visitor* visitor,
                                      String* string,
                                      int offset = 0);

  static Handle<FixedArray> CalculateLineEnds(Handle<String> string,
                                              bool include_ending_line);

  // Use the hash field to forward to the canonical internalized string
  // when deserializing an internalized string.
  inline void SetForwardedInternalizedString(String* string);
  inline String* GetForwardedInternalizedString();

 private:
  friend class Name;
  friend class StringTableInsertionKey;

  static Handle<String> SlowFlatten(Handle<ConsString> cons,
                                    PretenureFlag tenure);

  // Slow case of String::Equals.  This implementation works on any strings
  // but it is most efficient on strings that are almost flat.
  bool SlowEquals(String* other);

  static bool SlowEquals(Handle<String> one, Handle<String> two);

  // Slow case of AsArrayIndex.
  bool SlowAsArrayIndex(uint32_t* index);

  // Compute and set the hash code.
  uint32_t ComputeAndSetHash();

  DISALLOW_IMPLICIT_CONSTRUCTORS(String);
};


// The SeqString abstract class captures sequential string values.
class SeqString: public String {
 public:
  DECLARE_CAST(SeqString)

  // Layout description.
  static const int kHeaderSize = String::kSize;

  // Truncate the string in-place if possible and return the result.
  // In case of new_length == 0, the empty string is returned without
  // truncating the original string.
  MUST_USE_RESULT static Handle<String> Truncate(Handle<SeqString> string,
                                                 int new_length);
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SeqString);
};


// The OneByteString class captures sequential one-byte string objects.
// Each character in the OneByteString is an one-byte character.
class SeqOneByteString: public SeqString {
 public:
  static const bool kHasOneByteEncoding = true;

  // Dispatched behavior.
  inline uint16_t SeqOneByteStringGet(int index);
  inline void SeqOneByteStringSet(int index, uint16_t value);

  // Get the address of the characters in this string.
  inline Address GetCharsAddress();

  inline uint8_t* GetChars();

  DECLARE_CAST(SeqOneByteString)

  // Garbage collection support.  This method is called by the
  // garbage collector to compute the actual size of an OneByteString
  // instance.
  inline int SeqOneByteStringSize(InstanceType instance_type);

  // Computes the size for an OneByteString instance of a given length.
  static int SizeFor(int length) {
    return OBJECT_POINTER_ALIGN(kHeaderSize + length * kCharSize);
  }

  // Maximal memory usage for a single sequential one-byte string.
  static const int kMaxSize = 512 * MB - 1;
  STATIC_ASSERT((kMaxSize - kHeaderSize) >= String::kMaxLength);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SeqOneByteString);
};


// The TwoByteString class captures sequential unicode string objects.
// Each character in the TwoByteString is a two-byte uint16_t.
class SeqTwoByteString: public SeqString {
 public:
  static const bool kHasOneByteEncoding = false;

  // Dispatched behavior.
  inline uint16_t SeqTwoByteStringGet(int index);
  inline void SeqTwoByteStringSet(int index, uint16_t value);

  // Get the address of the characters in this string.
  inline Address GetCharsAddress();

  inline uc16* GetChars();

  // For regexp code.
  const uint16_t* SeqTwoByteStringGetData(unsigned start);

  DECLARE_CAST(SeqTwoByteString)

  // Garbage collection support.  This method is called by the
  // garbage collector to compute the actual size of a TwoByteString
  // instance.
  inline int SeqTwoByteStringSize(InstanceType instance_type);

  // Computes the size for a TwoByteString instance of a given length.
  static int SizeFor(int length) {
    return OBJECT_POINTER_ALIGN(kHeaderSize + length * kShortSize);
  }

  // Maximal memory usage for a single sequential two-byte string.
  static const int kMaxSize = 512 * MB - 1;
  STATIC_ASSERT(static_cast<int>((kMaxSize - kHeaderSize)/sizeof(uint16_t)) >=
               String::kMaxLength);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SeqTwoByteString);
};


// The ConsString class describes string values built by using the
// addition operator on strings.  A ConsString is a pair where the
// first and second components are pointers to other string values.
// One or both components of a ConsString can be pointers to other
// ConsStrings, creating a binary tree of ConsStrings where the leaves
// are non-ConsString string values.  The string value represented by
// a ConsString can be obtained by concatenating the leaf string
// values in a left-to-right depth-first traversal of the tree.
class ConsString: public String {
 public:
  // First string of the cons cell.
  inline String* first();
  // Doesn't check that the result is a string, even in debug mode.  This is
  // useful during GC where the mark bits confuse the checks.
  inline Object* unchecked_first();
  inline void set_first(String* first,
                        WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

  // Second string of the cons cell.
  inline String* second();
  // Doesn't check that the result is a string, even in debug mode.  This is
  // useful during GC where the mark bits confuse the checks.
  inline Object* unchecked_second();
  inline void set_second(String* second,
                         WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

  // Dispatched behavior.
  uint16_t ConsStringGet(int index);

  DECLARE_CAST(ConsString)

  // Layout description.
  static const int kFirstOffset = POINTER_SIZE_ALIGN(String::kSize);
  static const int kSecondOffset = kFirstOffset + kPointerSize;
  static const int kSize = kSecondOffset + kPointerSize;

  // Minimum length for a cons string.
  static const int kMinLength = 13;

  typedef FixedBodyDescriptor<kFirstOffset, kSecondOffset + kPointerSize, kSize>
          BodyDescriptor;

  DECLARE_VERIFIER(ConsString)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ConsString);
};


// The Sliced String class describes strings that are substrings of another
// sequential string.  The motivation is to save time and memory when creating
// a substring.  A Sliced String is described as a pointer to the parent,
// the offset from the start of the parent string and the length.  Using
// a Sliced String therefore requires unpacking of the parent string and
// adding the offset to the start address.  A substring of a Sliced String
// are not nested since the double indirection is simplified when creating
// such a substring.
// Currently missing features are:
//  - handling externalized parent strings
//  - external strings as parent
//  - truncating sliced string to enable otherwise unneeded parent to be GC'ed.
class SlicedString: public String {
 public:
  inline String* parent();
  inline void set_parent(String* parent,
                         WriteBarrierMode mode = UPDATE_WRITE_BARRIER);
  inline int offset() const;
  inline void set_offset(int offset);

  // Dispatched behavior.
  uint16_t SlicedStringGet(int index);

  DECLARE_CAST(SlicedString)

  // Layout description.
  static const int kParentOffset = POINTER_SIZE_ALIGN(String::kSize);
  static const int kOffsetOffset = kParentOffset + kPointerSize;
  static const int kSize = kOffsetOffset + kPointerSize;

  // Minimum length for a sliced string.
  static const int kMinLength = 13;

  typedef FixedBodyDescriptor<kParentOffset,
                              kOffsetOffset + kPointerSize, kSize>
          BodyDescriptor;

  DECLARE_VERIFIER(SlicedString)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SlicedString);
};


// The ExternalString class describes string values that are backed by
// a string resource that lies outside the V8 heap.  ExternalStrings
// consist of the length field common to all strings, a pointer to the
// external resource.  It is important to ensure (externally) that the
// resource is not deallocated while the ExternalString is live in the
// V8 heap.
//
// The API expects that all ExternalStrings are created through the
// API.  Therefore, ExternalStrings should not be used internally.
class ExternalString: public String {
 public:
  DECLARE_CAST(ExternalString)

  // Layout description.
  static const int kResourceOffset = POINTER_SIZE_ALIGN(String::kSize);
  static const int kShortSize = kResourceOffset + kPointerSize;
  static const int kResourceDataOffset = kResourceOffset + kPointerSize;
  static const int kSize = kResourceDataOffset + kPointerSize;

  static const int kMaxShortLength =
      (kShortSize - SeqString::kHeaderSize) / kCharSize;

  // Return whether external string is short (data pointer is not cached).
  inline bool is_short();

  STATIC_ASSERT(kResourceOffset == Internals::kStringResourceOffset);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ExternalString);
};


// The ExternalOneByteString class is an external string backed by an
// one-byte string.
class ExternalOneByteString : public ExternalString {
 public:
  static const bool kHasOneByteEncoding = true;

  typedef v8::String::ExternalOneByteStringResource Resource;

  // The underlying resource.
  inline const Resource* resource();
  inline void set_resource(const Resource* buffer);

  // Update the pointer cache to the external character array.
  // The cached pointer is always valid, as the external character array does =
  // not move during lifetime.  Deserialization is the only exception, after
  // which the pointer cache has to be refreshed.
  inline void update_data_cache();

  inline const uint8_t* GetChars();

  // Dispatched behavior.
  inline uint16_t ExternalOneByteStringGet(int index);

  DECLARE_CAST(ExternalOneByteString)

  // Garbage collection support.
  inline void ExternalOneByteStringIterateBody(ObjectVisitor* v);

  template <typename StaticVisitor>
  inline void ExternalOneByteStringIterateBody();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ExternalOneByteString);
};


// The ExternalTwoByteString class is an external string backed by a UTF-16
// encoded string.
class ExternalTwoByteString: public ExternalString {
 public:
  static const bool kHasOneByteEncoding = false;

  typedef v8::String::ExternalStringResource Resource;

  // The underlying string resource.
  inline const Resource* resource();
  inline void set_resource(const Resource* buffer);

  // Update the pointer cache to the external character array.
  // The cached pointer is always valid, as the external character array does =
  // not move during lifetime.  Deserialization is the only exception, after
  // which the pointer cache has to be refreshed.
  inline void update_data_cache();

  inline const uint16_t* GetChars();

  // Dispatched behavior.
  inline uint16_t ExternalTwoByteStringGet(int index);

  // For regexp code.
  inline const uint16_t* ExternalTwoByteStringGetData(unsigned start);

  DECLARE_CAST(ExternalTwoByteString)

  // Garbage collection support.
  inline void ExternalTwoByteStringIterateBody(ObjectVisitor* v);

  template<typename StaticVisitor>
  inline void ExternalTwoByteStringIterateBody();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ExternalTwoByteString);
};


// Utility superclass for stack-allocated objects that must be updated
// on gc.  It provides two ways for the gc to update instances, either
// iterating or updating after gc.
class Relocatable BASE_EMBEDDED {
 public:
  explicit inline Relocatable(Isolate* isolate);
  inline virtual ~Relocatable();
  virtual void IterateInstance(ObjectVisitor* v) { }
  virtual void PostGarbageCollection() { }

  static void PostGarbageCollectionProcessing(Isolate* isolate);
  static int ArchiveSpacePerThread();
  static char* ArchiveState(Isolate* isolate, char* to);
  static char* RestoreState(Isolate* isolate, char* from);
  static void Iterate(Isolate* isolate, ObjectVisitor* v);
  static void Iterate(ObjectVisitor* v, Relocatable* top);
  static char* Iterate(ObjectVisitor* v, char* t);

 private:
  Isolate* isolate_;
  Relocatable* prev_;
};


// A flat string reader provides random access to the contents of a
// string independent of the character width of the string.  The handle
// must be valid as long as the reader is being used.
class FlatStringReader : public Relocatable {
 public:
  FlatStringReader(Isolate* isolate, Handle<String> str);
  FlatStringReader(Isolate* isolate, Vector<const char> input);
  void PostGarbageCollection();
  inline uc32 Get(int index);
  template <typename Char>
  inline Char Get(int index);
  int length() { return length_; }
 private:
  String** str_;
  bool is_one_byte_;
  int length_;
  const void* start_;
};


// This maintains an off-stack representation of the stack frames required
// to traverse a ConsString, allowing an entirely iterative and restartable
// traversal of the entire string
class ConsStringIterator {
 public:
  inline ConsStringIterator() {}
  inline explicit ConsStringIterator(ConsString* cons_string, int offset = 0) {
    Reset(cons_string, offset);
  }
  inline void Reset(ConsString* cons_string, int offset = 0) {
    depth_ = 0;
    // Next will always return NULL.
    if (cons_string == NULL) return;
    Initialize(cons_string, offset);
  }
  // Returns NULL when complete.
  inline String* Next(int* offset_out) {
    *offset_out = 0;
    if (depth_ == 0) return NULL;
    return Continue(offset_out);
  }

 private:
  static const int kStackSize = 32;
  // Use a mask instead of doing modulo operations for stack wrapping.
  static const int kDepthMask = kStackSize-1;
  STATIC_ASSERT(IS_POWER_OF_TWO(kStackSize));
  static inline int OffsetForDepth(int depth);

  inline void PushLeft(ConsString* string);
  inline void PushRight(ConsString* string);
  inline void AdjustMaximumDepth();
  inline void Pop();
  inline bool StackBlown() { return maximum_depth_ - depth_ == kStackSize; }
  void Initialize(ConsString* cons_string, int offset);
  String* Continue(int* offset_out);
  String* NextLeaf(bool* blew_stack);
  String* Search(int* offset_out);

  // Stack must always contain only frames for which right traversal
  // has not yet been performed.
  ConsString* frames_[kStackSize];
  ConsString* root_;
  int depth_;
  int maximum_depth_;
  int consumed_;
  DISALLOW_COPY_AND_ASSIGN(ConsStringIterator);
};


class StringCharacterStream {
 public:
  inline StringCharacterStream(String* string,
                               int offset = 0);
  inline uint16_t GetNext();
  inline bool HasMore();
  inline void Reset(String* string, int offset = 0);
  inline void VisitOneByteString(const uint8_t* chars, int length);
  inline void VisitTwoByteString(const uint16_t* chars, int length);

 private:
  ConsStringIterator iter_;
  bool is_one_byte_;
  union {
    const uint8_t* buffer8_;
    const uint16_t* buffer16_;
  };
  const uint8_t* end_;
  DISALLOW_COPY_AND_ASSIGN(StringCharacterStream);
};


template <typename T>
class VectorIterator {
 public:
  VectorIterator(T* d, int l) : data_(Vector<const T>(d, l)), index_(0) { }
  explicit VectorIterator(Vector<const T> data) : data_(data), index_(0) { }
  T GetNext() { return data_[index_++]; }
  bool has_more() { return index_ < data_.length(); }
 private:
  Vector<const T> data_;
  int index_;
};


// The Oddball describes objects null, undefined, true, and false.
class Oddball: public HeapObject {
 public:
  // [to_string]: Cached to_string computed at startup.
  DECL_ACCESSORS(to_string, String)

  // [to_number]: Cached to_number computed at startup.
  DECL_ACCESSORS(to_number, Object)

  // [typeof]: Cached type_of computed at startup.
  DECL_ACCESSORS(type_of, String)

  inline byte kind() const;
  inline void set_kind(byte kind);

  // ES6 section 7.1.3 ToNumber for Boolean, Null, Undefined.
  MUST_USE_RESULT static inline Handle<Object> ToNumber(Handle<Oddball> input);

  DECLARE_CAST(Oddball)

  // Dispatched behavior.
  DECLARE_VERIFIER(Oddball)

  // Initialize the fields.
  static void Initialize(Isolate* isolate, Handle<Oddball> oddball,
                         const char* to_string, Handle<Object> to_number,
                         const char* type_of, byte kind);

  // Layout description.
  static const int kToStringOffset = HeapObject::kHeaderSize;
  static const int kToNumberOffset = kToStringOffset + kPointerSize;
  static const int kTypeOfOffset = kToNumberOffset + kPointerSize;
  static const int kKindOffset = kTypeOfOffset + kPointerSize;
  static const int kSize = kKindOffset + kPointerSize;

  static const byte kFalse = 0;
  static const byte kTrue = 1;
  static const byte kNotBooleanMask = ~1;
  static const byte kTheHole = 2;
  static const byte kNull = 3;
  static const byte kArgumentMarker = 4;
  static const byte kUndefined = 5;
  static const byte kUninitialized = 6;
  static const byte kOther = 7;
  static const byte kException = 8;

  typedef FixedBodyDescriptor<kToStringOffset, kTypeOfOffset + kPointerSize,
                              kSize> BodyDescriptor;

  STATIC_ASSERT(kKindOffset == Internals::kOddballKindOffset);
  STATIC_ASSERT(kNull == Internals::kNullOddballKind);
  STATIC_ASSERT(kUndefined == Internals::kUndefinedOddballKind);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Oddball);
};


class Cell: public HeapObject {
 public:
  // [value]: value of the cell.
  DECL_ACCESSORS(value, Object)

  DECLARE_CAST(Cell)

  static inline Cell* FromValueAddress(Address value) {
    Object* result = FromAddress(value - kValueOffset);
    return static_cast<Cell*>(result);
  }

  inline Address ValueAddress() {
    return address() + kValueOffset;
  }

  // Dispatched behavior.
  DECLARE_PRINTER(Cell)
  DECLARE_VERIFIER(Cell)

  // Layout description.
  static const int kValueOffset = HeapObject::kHeaderSize;
  static const int kSize = kValueOffset + kPointerSize;

  typedef FixedBodyDescriptor<kValueOffset,
                              kValueOffset + kPointerSize,
                              kSize> BodyDescriptor;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Cell);
};


class PropertyCell : public HeapObject {
 public:
  // [property_details]: details of the global property.
  DECL_ACCESSORS(property_details_raw, Object)
  // [value]: value of the global property.
  DECL_ACCESSORS(value, Object)
  // [dependent_code]: dependent code that depends on the type of the global
  // property.
  DECL_ACCESSORS(dependent_code, DependentCode)

  inline PropertyDetails property_details();
  inline void set_property_details(PropertyDetails details);

  PropertyCellConstantType GetConstantType();

  // Computes the new type of the cell's contents for the given value, but
  // without actually modifying the details.
  static PropertyCellType UpdatedType(Handle<PropertyCell> cell,
                                      Handle<Object> value,
                                      PropertyDetails details);
  static void UpdateCell(Handle<GlobalDictionary> dictionary, int entry,
                         Handle<Object> value, PropertyDetails details);

  static Handle<PropertyCell> InvalidateEntry(
      Handle<GlobalDictionary> dictionary, int entry);

  static void SetValueWithInvalidation(Handle<PropertyCell> cell,
                                       Handle<Object> new_value);

  DECLARE_CAST(PropertyCell)

  // Dispatched behavior.
  DECLARE_PRINTER(PropertyCell)
  DECLARE_VERIFIER(PropertyCell)

  // Layout description.
  static const int kDetailsOffset = HeapObject::kHeaderSize;
  static const int kValueOffset = kDetailsOffset + kPointerSize;
  static const int kDependentCodeOffset = kValueOffset + kPointerSize;
  static const int kSize = kDependentCodeOffset + kPointerSize;

  static const int kPointerFieldsBeginOffset = kValueOffset;
  static const int kPointerFieldsEndOffset = kSize;

  typedef FixedBodyDescriptor<kValueOffset,
                              kSize,
                              kSize> BodyDescriptor;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PropertyCell);
};


class WeakCell : public HeapObject {
 public:
  inline Object* value() const;

  // This should not be called by anyone except GC.
  inline void clear();

  // This should not be called by anyone except allocator.
  inline void initialize(HeapObject* value);

  inline bool cleared() const;

  DECL_ACCESSORS(next, Object)

  inline void clear_next(Heap* heap);

  inline bool next_cleared();

  DECLARE_CAST(WeakCell)

  DECLARE_PRINTER(WeakCell)
  DECLARE_VERIFIER(WeakCell)

  // Layout description.
  static const int kValueOffset = HeapObject::kHeaderSize;
  static const int kNextOffset = kValueOffset + kPointerSize;
  static const int kSize = kNextOffset + kPointerSize;

  typedef FixedBodyDescriptor<kValueOffset, kSize, kSize> BodyDescriptor;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WeakCell);
};


// The JSProxy describes EcmaScript Harmony proxies
class JSProxy: public JSReceiver {
 public:
  // [handler]: The handler property.
  DECL_ACCESSORS(handler, Object)

  // [hash]: The hash code property (undefined if not initialized yet).
  DECL_ACCESSORS(hash, Object)

  DECLARE_CAST(JSProxy)

  MUST_USE_RESULT static MaybeHandle<Object> GetPropertyWithHandler(
      Handle<JSProxy> proxy,
      Handle<Object> receiver,
      Handle<Name> name);

  // If the handler defines an accessor property with a setter, invoke it.
  // If it defines an accessor property without a setter, or a data property
  // that is read-only, throw. In all these cases set '*done' to true,
  // otherwise set it to false.
  MUST_USE_RESULT
  static MaybeHandle<Object> SetPropertyViaPrototypesWithHandler(
      Handle<JSProxy> proxy, Handle<Object> receiver, Handle<Name> name,
      Handle<Object> value, LanguageMode language_mode, bool* done);

  MUST_USE_RESULT static Maybe<PropertyAttributes>
      GetPropertyAttributesWithHandler(Handle<JSProxy> proxy,
                                       Handle<Object> receiver,
                                       Handle<Name> name);
  MUST_USE_RESULT static MaybeHandle<Object> SetPropertyWithHandler(
      Handle<JSProxy> proxy, Handle<Object> receiver, Handle<Name> name,
      Handle<Object> value, LanguageMode language_mode);

  // Turn the proxy into an (empty) JSObject.
  static void Fix(Handle<JSProxy> proxy);

  // Initializes the body after the handler slot.
  inline void InitializeBody(int object_size, Object* value);

  // Invoke a trap by name. If the trap does not exist on this's handler,
  // but derived_trap is non-NULL, invoke that instead.  May cause GC.
  MUST_USE_RESULT static MaybeHandle<Object> CallTrap(
      Handle<JSProxy> proxy,
      const char* name,
      Handle<Object> derived_trap,
      int argc,
      Handle<Object> args[]);

  // Dispatched behavior.
  DECLARE_PRINTER(JSProxy)
  DECLARE_VERIFIER(JSProxy)

  // Layout description. We add padding so that a proxy has the same
  // size as a virgin JSObject. This is essential for becoming a JSObject
  // upon freeze.
  static const int kHandlerOffset = HeapObject::kHeaderSize;
  static const int kHashOffset = kHandlerOffset + kPointerSize;
  static const int kPaddingOffset = kHashOffset + kPointerSize;
  static const int kSize = JSObject::kHeaderSize;
  static const int kHeaderSize = kPaddingOffset;
  static const int kPaddingSize = kSize - kPaddingOffset;

  STATIC_ASSERT(kPaddingSize >= 0);

  typedef FixedBodyDescriptor<kHandlerOffset,
                              kPaddingOffset,
                              kSize> BodyDescriptor;

 private:
  friend class JSReceiver;

  MUST_USE_RESULT static Maybe<bool> HasPropertyWithHandler(
      Handle<JSProxy> proxy, Handle<Name> name);

  MUST_USE_RESULT static MaybeHandle<Object> DeletePropertyWithHandler(
      Handle<JSProxy> proxy, Handle<Name> name, LanguageMode language_mode);

  MUST_USE_RESULT Object* GetIdentityHash();

  static Handle<Smi> GetOrCreateIdentityHash(Handle<JSProxy> proxy);

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSProxy);
};


class JSFunctionProxy: public JSProxy {
 public:
  // [call_trap]: The call trap.
  DECL_ACCESSORS(call_trap, JSReceiver)

  // [construct_trap]: The construct trap.
  DECL_ACCESSORS(construct_trap, Object)

  DECLARE_CAST(JSFunctionProxy)

  // Dispatched behavior.
  DECLARE_PRINTER(JSFunctionProxy)
  DECLARE_VERIFIER(JSFunctionProxy)

  // Layout description.
  static const int kCallTrapOffset = JSProxy::kPaddingOffset;
  static const int kConstructTrapOffset = kCallTrapOffset + kPointerSize;
  static const int kPaddingOffset = kConstructTrapOffset + kPointerSize;
  static const int kSize = JSFunction::kSize;
  static const int kPaddingSize = kSize - kPaddingOffset;

  STATIC_ASSERT(kPaddingSize >= 0);

  typedef FixedBodyDescriptor<kHandlerOffset,
                              kConstructTrapOffset + kPointerSize,
                              kSize> BodyDescriptor;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSFunctionProxy);
};


class JSCollection : public JSObject {
 public:
  // [table]: the backing hash table
  DECL_ACCESSORS(table, Object)

  static const int kTableOffset = JSObject::kHeaderSize;
  static const int kSize = kTableOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSCollection);
};


// The JSSet describes EcmaScript Harmony sets
class JSSet : public JSCollection {
 public:
  DECLARE_CAST(JSSet)

  static void Initialize(Handle<JSSet> set, Isolate* isolate);
  static void Clear(Handle<JSSet> set);

  // Dispatched behavior.
  DECLARE_PRINTER(JSSet)
  DECLARE_VERIFIER(JSSet)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSSet);
};


// The JSMap describes EcmaScript Harmony maps
class JSMap : public JSCollection {
 public:
  DECLARE_CAST(JSMap)

  static void Initialize(Handle<JSMap> map, Isolate* isolate);
  static void Clear(Handle<JSMap> map);

  // Dispatched behavior.
  DECLARE_PRINTER(JSMap)
  DECLARE_VERIFIER(JSMap)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSMap);
};


// OrderedHashTableIterator is an iterator that iterates over the keys and
// values of an OrderedHashTable.
//
// The iterator has a reference to the underlying OrderedHashTable data,
// [table], as well as the current [index] the iterator is at.
//
// When the OrderedHashTable is rehashed it adds a reference from the old table
// to the new table as well as storing enough data about the changes so that the
// iterator [index] can be adjusted accordingly.
//
// When the [Next] result from the iterator is requested, the iterator checks if
// there is a newer table that it needs to transition to.
template<class Derived, class TableType>
class OrderedHashTableIterator: public JSObject {
 public:
  // [table]: the backing hash table mapping keys to values.
  DECL_ACCESSORS(table, Object)

  // [index]: The index into the data table.
  DECL_ACCESSORS(index, Object)

  // [kind]: The kind of iteration this is. One of the [Kind] enum values.
  DECL_ACCESSORS(kind, Object)

#ifdef OBJECT_PRINT
  void OrderedHashTableIteratorPrint(std::ostream& os);  // NOLINT
#endif

  static const int kTableOffset = JSObject::kHeaderSize;
  static const int kIndexOffset = kTableOffset + kPointerSize;
  static const int kKindOffset = kIndexOffset + kPointerSize;
  static const int kSize = kKindOffset + kPointerSize;

  enum Kind {
    kKindKeys = 1,
    kKindValues = 2,
    kKindEntries = 3
  };

  // Whether the iterator has more elements. This needs to be called before
  // calling |CurrentKey| and/or |CurrentValue|.
  bool HasMore();

  // Move the index forward one.
  void MoveNext() {
    set_index(Smi::FromInt(Smi::cast(index())->value() + 1));
  }

  // Populates the array with the next key and value and then moves the iterator
  // forward.
  // This returns the |kind| or 0 if the iterator is already at the end.
  Smi* Next(JSArray* value_array);

  // Returns the current key of the iterator. This should only be called when
  // |HasMore| returns true.
  inline Object* CurrentKey();

 private:
  // Transitions the iterator to the non obsolete backing store. This is a NOP
  // if the [table] is not obsolete.
  void Transition();

  DISALLOW_IMPLICIT_CONSTRUCTORS(OrderedHashTableIterator);
};


class JSSetIterator: public OrderedHashTableIterator<JSSetIterator,
                                                     OrderedHashSet> {
 public:
  // Dispatched behavior.
  DECLARE_PRINTER(JSSetIterator)
  DECLARE_VERIFIER(JSSetIterator)

  DECLARE_CAST(JSSetIterator)

  // Called by |Next| to populate the array. This allows the subclasses to
  // populate the array differently.
  inline void PopulateValueArray(FixedArray* array);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSSetIterator);
};


class JSMapIterator: public OrderedHashTableIterator<JSMapIterator,
                                                     OrderedHashMap> {
 public:
  // Dispatched behavior.
  DECLARE_PRINTER(JSMapIterator)
  DECLARE_VERIFIER(JSMapIterator)

  DECLARE_CAST(JSMapIterator)

  // Called by |Next| to populate the array. This allows the subclasses to
  // populate the array differently.
  inline void PopulateValueArray(FixedArray* array);

 private:
  // Returns the current value of the iterator. This should only be called when
  // |HasMore| returns true.
  inline Object* CurrentValue();

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSMapIterator);
};


// ES6 section 25.1.1.3 The IteratorResult Interface
class JSIteratorResult final : public JSObject {
 public:
  // [done]: This is the result status of an iterator next method call.  If the
  // end of the iterator was reached done is true.  If the end was not reached
  // done is false and a [value] is available.
  DECL_ACCESSORS(done, Object)

  // [value]: If [done] is false, this is the current iteration element value.
  // If [done] is true, this is the return value of the iterator, if it supplied
  // one.  If the iterator does not have a return value, value is undefined.
  // In that case, the value property may be absent from the conforming object
  // if it does not inherit an explicit value property.
  DECL_ACCESSORS(value, Object)

  // Dispatched behavior.
  DECLARE_PRINTER(JSIteratorResult)
  DECLARE_VERIFIER(JSIteratorResult)

  DECLARE_CAST(JSIteratorResult)

  static const int kValueOffset = JSObject::kHeaderSize;
  static const int kDoneOffset = kValueOffset + kPointerSize;
  static const int kSize = kDoneOffset + kPointerSize;

  // Indices of in-object properties.
  static const int kValueIndex = 0;
  static const int kDoneIndex = 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSIteratorResult);
};


// Base class for both JSWeakMap and JSWeakSet
class JSWeakCollection: public JSObject {
 public:
  // [table]: the backing hash table mapping keys to values.
  DECL_ACCESSORS(table, Object)

  // [next]: linked list of encountered weak maps during GC.
  DECL_ACCESSORS(next, Object)

  static void Initialize(Handle<JSWeakCollection> collection, Isolate* isolate);
  static void Set(Handle<JSWeakCollection> collection, Handle<Object> key,
                  Handle<Object> value, int32_t hash);
  static bool Delete(Handle<JSWeakCollection> collection, Handle<Object> key,
                     int32_t hash);

  static const int kTableOffset = JSObject::kHeaderSize;
  static const int kNextOffset = kTableOffset + kPointerSize;
  static const int kSize = kNextOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSWeakCollection);
};


// The JSWeakMap describes EcmaScript Harmony weak maps
class JSWeakMap: public JSWeakCollection {
 public:
  DECLARE_CAST(JSWeakMap)

  // Dispatched behavior.
  DECLARE_PRINTER(JSWeakMap)
  DECLARE_VERIFIER(JSWeakMap)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSWeakMap);
};


// The JSWeakSet describes EcmaScript Harmony weak sets
class JSWeakSet: public JSWeakCollection {
 public:
  DECLARE_CAST(JSWeakSet)

  // Dispatched behavior.
  DECLARE_PRINTER(JSWeakSet)
  DECLARE_VERIFIER(JSWeakSet)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSWeakSet);
};


// Whether a JSArrayBuffer is a SharedArrayBuffer or not.
enum class SharedFlag { kNotShared, kShared };


class JSArrayBuffer: public JSObject {
 public:
  // [backing_store]: backing memory for this array
  DECL_ACCESSORS(backing_store, void)

  // [byte_length]: length in bytes
  DECL_ACCESSORS(byte_length, Object)

  inline uint32_t bit_field() const;
  inline void set_bit_field(uint32_t bits);

  inline bool is_external();
  inline void set_is_external(bool value);

  inline bool is_neuterable();
  inline void set_is_neuterable(bool value);

  inline bool was_neutered();
  inline void set_was_neutered(bool value);

  inline bool is_shared();
  inline void set_is_shared(bool value);

  DECLARE_CAST(JSArrayBuffer)

  void Neuter();

  static void Setup(Handle<JSArrayBuffer> array_buffer, Isolate* isolate,
                    bool is_external, void* data, size_t allocated_length,
                    SharedFlag shared = SharedFlag::kNotShared);

  static bool SetupAllocatingData(Handle<JSArrayBuffer> array_buffer,
                                  Isolate* isolate, size_t allocated_length,
                                  bool initialize = true,
                                  SharedFlag shared = SharedFlag::kNotShared);

  // Dispatched behavior.
  DECLARE_PRINTER(JSArrayBuffer)
  DECLARE_VERIFIER(JSArrayBuffer)

  static const int kByteLengthOffset = JSObject::kHeaderSize;

  // NOTE: GC will visit objects fields:
  // 1. From JSObject::BodyDescriptor::kStartOffset to kByteLengthOffset +
  //    kPointerSize
  // 2. From start of the internal fields and up to the end of them
  static const int kBackingStoreOffset = kByteLengthOffset + kPointerSize;
  static const int kBitFieldSlot = kBackingStoreOffset + kPointerSize;
#if V8_TARGET_LITTLE_ENDIAN || !V8_HOST_ARCH_64_BIT
  static const int kBitFieldOffset = kBitFieldSlot;
#else
  static const int kBitFieldOffset = kBitFieldSlot + kIntSize;
#endif
  static const int kSize = kBitFieldSlot + kPointerSize;

  static const int kSizeWithInternalFields =
      kSize + v8::ArrayBuffer::kInternalFieldCount * kPointerSize;

  template <typename StaticVisitor>
  static inline void JSArrayBufferIterateBody(Heap* heap, HeapObject* obj);

  static inline void JSArrayBufferIterateBody(HeapObject* obj,
                                              ObjectVisitor* v);

  class IsExternal : public BitField<bool, 1, 1> {};
  class IsNeuterable : public BitField<bool, 2, 1> {};
  class WasNeutered : public BitField<bool, 3, 1> {};
  class IsShared : public BitField<bool, 4, 1> {};

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSArrayBuffer);
};


class JSArrayBufferView: public JSObject {
 public:
  // [buffer]: ArrayBuffer that this typed array views.
  DECL_ACCESSORS(buffer, Object)

  // [byte_offset]: offset of typed array in bytes.
  DECL_ACCESSORS(byte_offset, Object)

  // [byte_length]: length of typed array in bytes.
  DECL_ACCESSORS(byte_length, Object)

  DECLARE_CAST(JSArrayBufferView)

  DECLARE_VERIFIER(JSArrayBufferView)

  inline bool WasNeutered() const;

  static const int kBufferOffset = JSObject::kHeaderSize;
  static const int kByteOffsetOffset = kBufferOffset + kPointerSize;
  static const int kByteLengthOffset = kByteOffsetOffset + kPointerSize;
  static const int kViewSize = kByteLengthOffset + kPointerSize;

 private:
#ifdef VERIFY_HEAP
  DECL_ACCESSORS(raw_byte_offset, Object)
  DECL_ACCESSORS(raw_byte_length, Object)
#endif

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSArrayBufferView);
};


class JSTypedArray: public JSArrayBufferView {
 public:
  // [length]: length of typed array in elements.
  DECL_ACCESSORS(length, Object)
  inline uint32_t length_value() const;

  DECLARE_CAST(JSTypedArray)

  ExternalArrayType type();
  size_t element_size();

  Handle<JSArrayBuffer> GetBuffer();

  // Dispatched behavior.
  DECLARE_PRINTER(JSTypedArray)
  DECLARE_VERIFIER(JSTypedArray)

  static const int kLengthOffset = kViewSize + kPointerSize;
  static const int kSize = kLengthOffset + kPointerSize;

  static const int kSizeWithInternalFields =
      kSize + v8::ArrayBufferView::kInternalFieldCount * kPointerSize;

 private:
  static Handle<JSArrayBuffer> MaterializeArrayBuffer(
      Handle<JSTypedArray> typed_array);
#ifdef VERIFY_HEAP
  DECL_ACCESSORS(raw_length, Object)
#endif

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSTypedArray);
};


class JSDataView: public JSArrayBufferView {
 public:
  DECLARE_CAST(JSDataView)

  // Dispatched behavior.
  DECLARE_PRINTER(JSDataView)
  DECLARE_VERIFIER(JSDataView)

  static const int kSize = kViewSize;

  static const int kSizeWithInternalFields =
      kSize + v8::ArrayBufferView::kInternalFieldCount * kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSDataView);
};


// Foreign describes objects pointing from JavaScript to C structures.
class Foreign: public HeapObject {
 public:
  // [address]: field containing the address.
  inline Address foreign_address();
  inline void set_foreign_address(Address value);

  DECLARE_CAST(Foreign)

  // Dispatched behavior.
  inline void ForeignIterateBody(ObjectVisitor* v);

  template<typename StaticVisitor>
  inline void ForeignIterateBody();

  // Dispatched behavior.
  DECLARE_PRINTER(Foreign)
  DECLARE_VERIFIER(Foreign)

  // Layout description.

  static const int kForeignAddressOffset = HeapObject::kHeaderSize;
  static const int kSize = kForeignAddressOffset + kPointerSize;

  STATIC_ASSERT(kForeignAddressOffset == Internals::kForeignAddressOffset);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Foreign);
};


// The JSArray describes JavaScript Arrays
//  Such an array can be in one of two modes:
//    - fast, backing storage is a FixedArray and length <= elements.length();
//       Please note: push and pop can be used to grow and shrink the array.
//    - slow, backing storage is a HashTable with numbers as keys.
class JSArray: public JSObject {
 public:
  // [length]: The length property.
  DECL_ACCESSORS(length, Object)

  // Overload the length setter to skip write barrier when the length
  // is set to a smi. This matches the set function on FixedArray.
  inline void set_length(Smi* length);

  static bool HasReadOnlyLength(Handle<JSArray> array);
  static bool WouldChangeReadOnlyLength(Handle<JSArray> array, uint32_t index);
  static MaybeHandle<Object> ReadOnlyLengthError(Handle<JSArray> array);

  // Initialize the array with the given capacity. The function may
  // fail due to out-of-memory situations, but only if the requested
  // capacity is non-zero.
  static void Initialize(Handle<JSArray> array, int capacity, int length = 0);

  // If the JSArray has fast elements, and new_length would result in
  // normalization, returns true.
  bool SetLengthWouldNormalize(uint32_t new_length);
  static inline bool SetLengthWouldNormalize(Heap* heap, uint32_t new_length);

  // Initializes the array to a certain length.
  inline bool AllowsSetLength();

  static void SetLength(Handle<JSArray> array, uint32_t length);
  // Same as above but will also queue splice records if |array| is observed.
  static MaybeHandle<Object> ObservableSetLength(Handle<JSArray> array,
                                                 uint32_t length);

  // Set the content of the array to the content of storage.
  static inline void SetContent(Handle<JSArray> array,
                                Handle<FixedArrayBase> storage);

  DECLARE_CAST(JSArray)

  // Dispatched behavior.
  DECLARE_PRINTER(JSArray)
  DECLARE_VERIFIER(JSArray)

  // Number of element slots to pre-allocate for an empty array.
  static const int kPreallocatedArrayElements = 4;

  // Layout description.
  static const int kLengthOffset = JSObject::kHeaderSize;
  static const int kSize = kLengthOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSArray);
};


Handle<Object> CacheInitialJSArrayMaps(Handle<Context> native_context,
                                       Handle<Map> initial_map);


// JSRegExpResult is just a JSArray with a specific initial map.
// This initial map adds in-object properties for "index" and "input"
// properties, as assigned by RegExp.prototype.exec, which allows
// faster creation of RegExp exec results.
// This class just holds constants used when creating the result.
// After creation the result must be treated as a JSArray in all regards.
class JSRegExpResult: public JSArray {
 public:
  // Offsets of object fields.
  static const int kIndexOffset = JSArray::kSize;
  static const int kInputOffset = kIndexOffset + kPointerSize;
  static const int kSize = kInputOffset + kPointerSize;
  // Indices of in-object properties.
  static const int kIndexIndex = 0;
  static const int kInputIndex = 1;
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(JSRegExpResult);
};


class AccessorInfo: public Struct {
 public:
  DECL_ACCESSORS(name, Object)
  DECL_ACCESSORS(flag, Smi)
  DECL_ACCESSORS(expected_receiver_type, Object)

  inline bool all_can_read();
  inline void set_all_can_read(bool value);

  inline bool all_can_write();
  inline void set_all_can_write(bool value);

  inline bool is_special_data_property();
  inline void set_is_special_data_property(bool value);

  inline PropertyAttributes property_attributes();
  inline void set_property_attributes(PropertyAttributes attributes);

  // Checks whether the given receiver is compatible with this accessor.
  static bool IsCompatibleReceiverMap(Isolate* isolate,
                                      Handle<AccessorInfo> info,
                                      Handle<Map> map);
  inline bool IsCompatibleReceiver(Object* receiver);

  DECLARE_CAST(AccessorInfo)

  // Dispatched behavior.
  DECLARE_VERIFIER(AccessorInfo)

  // Append all descriptors to the array that are not already there.
  // Return number added.
  static int AppendUnique(Handle<Object> descriptors,
                          Handle<FixedArray> array,
                          int valid_descriptors);

  static const int kNameOffset = HeapObject::kHeaderSize;
  static const int kFlagOffset = kNameOffset + kPointerSize;
  static const int kExpectedReceiverTypeOffset = kFlagOffset + kPointerSize;
  static const int kSize = kExpectedReceiverTypeOffset + kPointerSize;

 private:
  inline bool HasExpectedReceiverType();

  // Bit positions in flag.
  static const int kAllCanReadBit = 0;
  static const int kAllCanWriteBit = 1;
  static const int kSpecialDataProperty = 2;
  class AttributesField : public BitField<PropertyAttributes, 3, 3> {};

  DISALLOW_IMPLICIT_CONSTRUCTORS(AccessorInfo);
};


// An accessor must have a getter, but can have no setter.
//
// When setting a property, V8 searches accessors in prototypes.
// If an accessor was found and it does not have a setter,
// the request is ignored.
//
// If the accessor in the prototype has the READ_ONLY property attribute, then
// a new value is added to the derived object when the property is set.
// This shadows the accessor in the prototype.
class ExecutableAccessorInfo: public AccessorInfo {
 public:
  DECL_ACCESSORS(getter, Object)
  DECL_ACCESSORS(setter, Object)
  DECL_ACCESSORS(data, Object)

  DECLARE_CAST(ExecutableAccessorInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(ExecutableAccessorInfo)
  DECLARE_VERIFIER(ExecutableAccessorInfo)

  static const int kGetterOffset = AccessorInfo::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDataOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  static void ClearSetter(Handle<ExecutableAccessorInfo> info);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ExecutableAccessorInfo);
};


// Support for JavaScript accessors: A pair of a getter and a setter. Each
// accessor can either be
//   * a pointer to a JavaScript function or proxy: a real accessor
//   * undefined: considered an accessor by the spec, too, strangely enough
//   * the hole: an accessor which has not been set
//   * a pointer to a map: a transition used to ensure map sharing
class AccessorPair: public Struct {
 public:
  DECL_ACCESSORS(getter, Object)
  DECL_ACCESSORS(setter, Object)

  DECLARE_CAST(AccessorPair)

  static Handle<AccessorPair> Copy(Handle<AccessorPair> pair);

  inline Object* get(AccessorComponent component);
  inline void set(AccessorComponent component, Object* value);

  // Note: Returns undefined instead in case of a hole.
  Object* GetComponent(AccessorComponent component);

  // Set both components, skipping arguments which are a JavaScript null.
  inline void SetComponents(Object* getter, Object* setter);

  inline bool Equals(AccessorPair* pair);
  inline bool Equals(Object* getter_value, Object* setter_value);

  inline bool ContainsAccessor();

  // Dispatched behavior.
  DECLARE_PRINTER(AccessorPair)
  DECLARE_VERIFIER(AccessorPair)

  static const int kGetterOffset = HeapObject::kHeaderSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kSize = kSetterOffset + kPointerSize;

 private:
  // Strangely enough, in addition to functions and harmony proxies, the spec
  // requires us to consider undefined as a kind of accessor, too:
  //    var obj = {};
  //    Object.defineProperty(obj, "foo", {get: undefined});
  //    assertTrue("foo" in obj);
  inline bool IsJSAccessor(Object* obj);

  DISALLOW_IMPLICIT_CONSTRUCTORS(AccessorPair);
};


class AccessCheckInfo: public Struct {
 public:
  DECL_ACCESSORS(named_callback, Object)
  DECL_ACCESSORS(indexed_callback, Object)
  DECL_ACCESSORS(data, Object)

  DECLARE_CAST(AccessCheckInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(AccessCheckInfo)
  DECLARE_VERIFIER(AccessCheckInfo)

  static const int kNamedCallbackOffset   = HeapObject::kHeaderSize;
  static const int kIndexedCallbackOffset = kNamedCallbackOffset + kPointerSize;
  static const int kDataOffset = kIndexedCallbackOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AccessCheckInfo);
};


class InterceptorInfo: public Struct {
 public:
  DECL_ACCESSORS(getter, Object)
  DECL_ACCESSORS(setter, Object)
  DECL_ACCESSORS(query, Object)
  DECL_ACCESSORS(deleter, Object)
  DECL_ACCESSORS(enumerator, Object)
  DECL_ACCESSORS(data, Object)
  DECL_BOOLEAN_ACCESSORS(can_intercept_symbols)
  DECL_BOOLEAN_ACCESSORS(all_can_read)
  DECL_BOOLEAN_ACCESSORS(non_masking)

  inline int flags() const;
  inline void set_flags(int flags);

  DECLARE_CAST(InterceptorInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(InterceptorInfo)
  DECLARE_VERIFIER(InterceptorInfo)

  static const int kGetterOffset = HeapObject::kHeaderSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kQueryOffset = kSetterOffset + kPointerSize;
  static const int kDeleterOffset = kQueryOffset + kPointerSize;
  static const int kEnumeratorOffset = kDeleterOffset + kPointerSize;
  static const int kDataOffset = kEnumeratorOffset + kPointerSize;
  static const int kFlagsOffset = kDataOffset + kPointerSize;
  static const int kSize = kFlagsOffset + kPointerSize;

  static const int kCanInterceptSymbolsBit = 0;
  static const int kAllCanReadBit = 1;
  static const int kNonMasking = 2;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(InterceptorInfo);
};


class CallHandlerInfo: public Struct {
 public:
  DECL_ACCESSORS(callback, Object)
  DECL_ACCESSORS(data, Object)

  DECLARE_CAST(CallHandlerInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(CallHandlerInfo)
  DECLARE_VERIFIER(CallHandlerInfo)

  static const int kCallbackOffset = HeapObject::kHeaderSize;
  static const int kDataOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CallHandlerInfo);
};


class TemplateInfo: public Struct {
 public:
  DECL_ACCESSORS(tag, Object)
  inline int number_of_properties() const;
  inline void set_number_of_properties(int value);
  DECL_ACCESSORS(property_list, Object)
  DECL_ACCESSORS(property_accessors, Object)

  DECLARE_VERIFIER(TemplateInfo)

  static const int kTagOffset = HeapObject::kHeaderSize;
  static const int kNumberOfProperties = kTagOffset + kPointerSize;
  static const int kPropertyListOffset = kNumberOfProperties + kPointerSize;
  static const int kPropertyAccessorsOffset =
      kPropertyListOffset + kPointerSize;
  static const int kHeaderSize = kPropertyAccessorsOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TemplateInfo);
};


class FunctionTemplateInfo: public TemplateInfo {
 public:
  DECL_ACCESSORS(serial_number, Object)
  DECL_ACCESSORS(call_code, Object)
  DECL_ACCESSORS(prototype_template, Object)
  DECL_ACCESSORS(parent_template, Object)
  DECL_ACCESSORS(named_property_handler, Object)
  DECL_ACCESSORS(indexed_property_handler, Object)
  DECL_ACCESSORS(instance_template, Object)
  DECL_ACCESSORS(class_name, Object)
  DECL_ACCESSORS(signature, Object)
  DECL_ACCESSORS(instance_call_handler, Object)
  DECL_ACCESSORS(access_check_info, Object)
  DECL_ACCESSORS(flag, Smi)

  inline int length() const;
  inline void set_length(int value);

  // Following properties use flag bits.
  DECL_BOOLEAN_ACCESSORS(hidden_prototype)
  DECL_BOOLEAN_ACCESSORS(undetectable)
  // If the bit is set, object instances created by this function
  // requires access check.
  DECL_BOOLEAN_ACCESSORS(needs_access_check)
  DECL_BOOLEAN_ACCESSORS(read_only_prototype)
  DECL_BOOLEAN_ACCESSORS(remove_prototype)
  DECL_BOOLEAN_ACCESSORS(do_not_cache)
  DECL_BOOLEAN_ACCESSORS(instantiated)
  DECL_BOOLEAN_ACCESSORS(accept_any_receiver)

  DECLARE_CAST(FunctionTemplateInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(FunctionTemplateInfo)
  DECLARE_VERIFIER(FunctionTemplateInfo)

  static const int kSerialNumberOffset = TemplateInfo::kHeaderSize;
  static const int kCallCodeOffset = kSerialNumberOffset + kPointerSize;
  static const int kPrototypeTemplateOffset =
      kCallCodeOffset + kPointerSize;
  static const int kParentTemplateOffset =
      kPrototypeTemplateOffset + kPointerSize;
  static const int kNamedPropertyHandlerOffset =
      kParentTemplateOffset + kPointerSize;
  static const int kIndexedPropertyHandlerOffset =
      kNamedPropertyHandlerOffset + kPointerSize;
  static const int kInstanceTemplateOffset =
      kIndexedPropertyHandlerOffset + kPointerSize;
  static const int kClassNameOffset = kInstanceTemplateOffset + kPointerSize;
  static const int kSignatureOffset = kClassNameOffset + kPointerSize;
  static const int kInstanceCallHandlerOffset = kSignatureOffset + kPointerSize;
  static const int kAccessCheckInfoOffset =
      kInstanceCallHandlerOffset + kPointerSize;
  static const int kFlagOffset = kAccessCheckInfoOffset + kPointerSize;
  static const int kLengthOffset = kFlagOffset + kPointerSize;
  static const int kSize = kLengthOffset + kPointerSize;

  // Returns true if |object| is an instance of this function template.
  bool IsTemplateFor(Object* object);
  bool IsTemplateFor(Map* map);

  // Returns the holder JSObject if the function can legally be called with this
  // receiver.  Returns Heap::null_value() if the call is illegal.
  Object* GetCompatibleReceiver(Isolate* isolate, Object* receiver);

 private:
  // Bit position in the flag, from least significant bit position.
  static const int kHiddenPrototypeBit   = 0;
  static const int kUndetectableBit      = 1;
  static const int kNeedsAccessCheckBit  = 2;
  static const int kReadOnlyPrototypeBit = 3;
  static const int kRemovePrototypeBit   = 4;
  static const int kDoNotCacheBit        = 5;
  static const int kInstantiatedBit      = 6;
  static const int kAcceptAnyReceiver = 7;

  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionTemplateInfo);
};


class ObjectTemplateInfo: public TemplateInfo {
 public:
  DECL_ACCESSORS(constructor, Object)
  DECL_ACCESSORS(internal_field_count, Object)

  DECLARE_CAST(ObjectTemplateInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(ObjectTemplateInfo)
  DECLARE_VERIFIER(ObjectTemplateInfo)

  static const int kConstructorOffset = TemplateInfo::kHeaderSize;
  static const int kInternalFieldCountOffset =
      kConstructorOffset + kPointerSize;
  static const int kSize = kInternalFieldCountOffset + kPointerSize;
};


class TypeSwitchInfo: public Struct {
 public:
  DECL_ACCESSORS(types, Object)

  DECLARE_CAST(TypeSwitchInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(TypeSwitchInfo)
  DECLARE_VERIFIER(TypeSwitchInfo)

  static const int kTypesOffset = Struct::kHeaderSize;
  static const int kSize        = kTypesOffset + kPointerSize;
};


// The DebugInfo class holds additional information for a function being
// debugged.
class DebugInfo: public Struct {
 public:
  // The shared function info for the source being debugged.
  DECL_ACCESSORS(shared, SharedFunctionInfo)
  // Code object for the patched code. This code object is the code object
  // currently active for the function.
  DECL_ACCESSORS(code, Code)
  // Fixed array holding status information for each active break point.
  DECL_ACCESSORS(break_points, FixedArray)

  // Check if there is a break point at a code position.
  bool HasBreakPoint(int code_position);
  // Get the break point info object for a code position.
  Object* GetBreakPointInfo(int code_position);
  // Clear a break point.
  static void ClearBreakPoint(Handle<DebugInfo> debug_info,
                              int code_position,
                              Handle<Object> break_point_object);
  // Set a break point.
  static void SetBreakPoint(Handle<DebugInfo> debug_info, int code_position,
                            int source_position, int statement_position,
                            Handle<Object> break_point_object);
  // Get the break point objects for a code position.
  Handle<Object> GetBreakPointObjects(int code_position);
  // Find the break point info holding this break point object.
  static Handle<Object> FindBreakPointInfo(Handle<DebugInfo> debug_info,
                                           Handle<Object> break_point_object);
  // Get the number of break points for this function.
  int GetBreakPointCount();

  DECLARE_CAST(DebugInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(DebugInfo)
  DECLARE_VERIFIER(DebugInfo)

  static const int kSharedFunctionInfoIndex = Struct::kHeaderSize;
  static const int kCodeIndex = kSharedFunctionInfoIndex + kPointerSize;
  static const int kBreakPointsStateIndex = kCodeIndex + kPointerSize;
  static const int kSize = kBreakPointsStateIndex + kPointerSize;

  static const int kEstimatedNofBreakPointsInFunction = 16;

 private:
  static const int kNoBreakPointInfo = -1;

  // Lookup the index in the break_points array for a code position.
  int GetBreakPointInfoIndex(int code_position);

  DISALLOW_IMPLICIT_CONSTRUCTORS(DebugInfo);
};


// The BreakPointInfo class holds information for break points set in a
// function. The DebugInfo object holds a BreakPointInfo object for each code
// position with one or more break points.
class BreakPointInfo: public Struct {
 public:
  // The position in the code for the break point.
  DECL_ACCESSORS(code_position, Smi)
  // The position in the source for the break position.
  DECL_ACCESSORS(source_position, Smi)
  // The position in the source for the last statement before this break
  // position.
  DECL_ACCESSORS(statement_position, Smi)
  // List of related JavaScript break points.
  DECL_ACCESSORS(break_point_objects, Object)

  // Removes a break point.
  static void ClearBreakPoint(Handle<BreakPointInfo> info,
                              Handle<Object> break_point_object);
  // Set a break point.
  static void SetBreakPoint(Handle<BreakPointInfo> info,
                            Handle<Object> break_point_object);
  // Check if break point info has this break point object.
  static bool HasBreakPointObject(Handle<BreakPointInfo> info,
                                  Handle<Object> break_point_object);
  // Get the number of break points for this code position.
  int GetBreakPointCount();

  DECLARE_CAST(BreakPointInfo)

  // Dispatched behavior.
  DECLARE_PRINTER(BreakPointInfo)
  DECLARE_VERIFIER(BreakPointInfo)

  static const int kCodePositionIndex = Struct::kHeaderSize;
  static const int kSourcePositionIndex = kCodePositionIndex + kPointerSize;
  static const int kStatementPositionIndex =
      kSourcePositionIndex + kPointerSize;
  static const int kBreakPointObjectsIndex =
      kStatementPositionIndex + kPointerSize;
  static const int kSize = kBreakPointObjectsIndex + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BreakPointInfo);
};


#undef DECL_BOOLEAN_ACCESSORS
#undef DECL_ACCESSORS
#undef DECLARE_CAST
#undef DECLARE_VERIFIER

#define VISITOR_SYNCHRONIZATION_TAGS_LIST(V)                               \
  V(kStringTable, "string_table", "(Internalized strings)")                \
  V(kExternalStringsTable, "external_strings_table", "(External strings)") \
  V(kStrongRootList, "strong_root_list", "(Strong roots)")                 \
  V(kSmiRootList, "smi_root_list", "(Smi roots)")                          \
  V(kBootstrapper, "bootstrapper", "(Bootstrapper)")                       \
  V(kTop, "top", "(Isolate)")                                              \
  V(kRelocatable, "relocatable", "(Relocatable)")                          \
  V(kDebug, "debug", "(Debugger)")                                         \
  V(kCompilationCache, "compilationcache", "(Compilation cache)")          \
  V(kHandleScope, "handlescope", "(Handle scope)")                         \
  V(kBuiltins, "builtins", "(Builtins)")                                   \
  V(kGlobalHandles, "globalhandles", "(Global handles)")                   \
  V(kEternalHandles, "eternalhandles", "(Eternal handles)")                \
  V(kThreadManager, "threadmanager", "(Thread manager)")                   \
  V(kStrongRoots, "strong roots", "(Strong roots)")                        \
  V(kExtensions, "Extensions", "(Extensions)")

class VisitorSynchronization : public AllStatic {
 public:
#define DECLARE_ENUM(enum_item, ignore1, ignore2) enum_item,
  enum SyncTag {
    VISITOR_SYNCHRONIZATION_TAGS_LIST(DECLARE_ENUM)
    kNumberOfSyncTags
  };
#undef DECLARE_ENUM

  static const char* const kTags[kNumberOfSyncTags];
  static const char* const kTagNames[kNumberOfSyncTags];
};

// Abstract base class for visiting, and optionally modifying, the
// pointers contained in Objects. Used in GC and serialization/deserialization.
class ObjectVisitor BASE_EMBEDDED {
 public:
  virtual ~ObjectVisitor() {}

  // Visits a contiguous arrays of pointers in the half-open range
  // [start, end). Any or all of the values may be modified on return.
  virtual void VisitPointers(Object** start, Object** end) = 0;

  // Handy shorthand for visiting a single pointer.
  virtual void VisitPointer(Object** p) { VisitPointers(p, p + 1); }

  // Visit weak next_code_link in Code object.
  virtual void VisitNextCodeLink(Object** p) { VisitPointers(p, p + 1); }

  // To allow lazy clearing of inline caches the visitor has
  // a rich interface for iterating over Code objects..

  // Visits a code target in the instruction stream.
  virtual void VisitCodeTarget(RelocInfo* rinfo);

  // Visits a code entry in a JS function.
  virtual void VisitCodeEntry(Address entry_address);

  // Visits a global property cell reference in the instruction stream.
  virtual void VisitCell(RelocInfo* rinfo);

  // Visits a runtime entry in the instruction stream.
  virtual void VisitRuntimeEntry(RelocInfo* rinfo) {}

  // Visits the resource of an one-byte or two-byte string.
  virtual void VisitExternalOneByteString(
      v8::String::ExternalOneByteStringResource** resource) {}
  virtual void VisitExternalTwoByteString(
      v8::String::ExternalStringResource** resource) {}

  // Visits a debug call target in the instruction stream.
  virtual void VisitDebugTarget(RelocInfo* rinfo);

  // Visits the byte sequence in a function's prologue that contains information
  // about the code's age.
  virtual void VisitCodeAgeSequence(RelocInfo* rinfo);

  // Visit pointer embedded into a code object.
  virtual void VisitEmbeddedPointer(RelocInfo* rinfo);

  // Visits an external reference embedded into a code object.
  virtual void VisitExternalReference(RelocInfo* rinfo);

  // Visits an external reference.
  virtual void VisitExternalReference(Address* p) {}

  // Visits an (encoded) internal reference.
  virtual void VisitInternalReference(RelocInfo* rinfo) {}

  // Visits a handle that has an embedder-assigned class ID.
  virtual void VisitEmbedderReference(Object** p, uint16_t class_id) {}

  // Intended for serialization/deserialization checking: insert, or
  // check for the presence of, a tag at this position in the stream.
  // Also used for marking up GC roots in heap snapshots.
  virtual void Synchronize(VisitorSynchronization::SyncTag tag) {}
};


class StructBodyDescriptor : public
  FlexibleBodyDescriptor<HeapObject::kHeaderSize> {
 public:
  static inline int SizeOf(Map* map, HeapObject* object);
};


// BooleanBit is a helper class for setting and getting a bit in an
// integer or Smi.
class BooleanBit : public AllStatic {
 public:
  static inline bool get(Smi* smi, int bit_position) {
    return get(smi->value(), bit_position);
  }

  static inline bool get(int value, int bit_position) {
    return (value & (1 << bit_position)) != 0;
  }

  static inline Smi* set(Smi* smi, int bit_position, bool v) {
    return Smi::FromInt(set(smi->value(), bit_position, v));
  }

  static inline int set(int value, int bit_position, bool v) {
    if (v) {
      value |= (1 << bit_position);
    } else {
      value &= ~(1 << bit_position);
    }
    return value;
  }
};


class KeyAccumulator final BASE_EMBEDDED {
 public:
  explicit KeyAccumulator(Isolate* isolate) : isolate_(isolate), length_(0) {}

  void AddKey(Handle<Object> key, int check_limit);
  void AddKeys(Handle<FixedArray> array, FixedArray::KeyFilter filter);
  void AddKeys(Handle<JSObject> array, FixedArray::KeyFilter filter);
  void PrepareForComparisons(int count);
  Handle<FixedArray> GetKeys();

  int GetLength() { return length_; }

 private:
  void EnsureCapacity(int capacity);
  void Grow();

  Isolate* isolate_;
  Handle<FixedArray> keys_;
  Handle<OrderedHashSet> set_;
  int length_;
  DISALLOW_COPY_AND_ASSIGN(KeyAccumulator);
};
} }  // namespace v8::internal

#endif  // V8_OBJECTS_H_
