#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ComputeCxx/AGTargetConditionals.h>

#ifndef __has_include
#define __has_include(x) 0
#endif
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif

#define _AG_STRINGIFY(_x) #_x

#if !defined(AG_EXTERN_C_BEGIN)
#if defined(__cplusplus)
#define AG_EXTERN_C_BEGIN extern "C" {
#define AG_EXTERN_C_END   }
#else
#define AG_EXTERN_C_BEGIN
#define AG_EXTERN_C_END
#endif
#endif

#if __GNUC__
#define AG_EXPORT extern __attribute__((__visibility__("default")))
#else
#define AG_EXPORT extern
#endif

#if __GNUC__
#define AG_INLINE static __inline__
#else
#define AG_INLINE static inline
#endif

#ifndef AG_RETURNS_RETAINED
#if __has_feature(attribute_cf_returns_retained)
#define AG_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
#define AG_RETURNS_RETAINED
#endif
#endif

#ifndef AG_IMPLICIT_BRIDGING_ENABLED
#if __has_feature(arc_cf_code_audited)
#define AG_IMPLICIT_BRIDGING_ENABLED _Pragma("clang arc_cf_code_audited begin")
#else
#define AG_IMPLICIT_BRIDGING_ENABLED
#endif
#endif

#ifndef AG_IMPLICIT_BRIDGING_DISABLED
#if __has_feature(arc_cf_code_audited)
#define AG_IMPLICIT_BRIDGING_DISABLED _Pragma("clang arc_cf_code_audited end")
#else
#define AG_IMPLICIT_BRIDGING_DISABLED
#endif
#endif

#if __has_attribute(objc_bridge) && __has_feature(objc_bridge_id) && __has_feature(objc_bridge_id_on_typedefs)

#ifdef __OBJC__
@class NSArray;
@class NSAttributedString;
@class NSString;
@class NSNull;
@class NSCharacterSet;
@class NSData;
@class NSDate;
@class NSTimeZone;
@class NSDictionary;
@class NSError;
@class NSLocale;
@class NSNumber;
@class NSSet;
@class NSURL;
#endif

#define AG_BRIDGED_TYPE(T)        __attribute__((objc_bridge(T)))
#define AG_BRIDGED_MUTABLE_TYPE(T)    __attribute__((objc_bridge_mutable(T)))
#define AG_RELATED_TYPE(T,C,I)        __attribute__((objc_bridge_related(T,C,I)))
#else
#define AG_BRIDGED_TYPE(T)
#define AG_BRIDGED_MUTABLE_TYPE(T)
#define AG_RELATED_TYPE(T,C,I)
#endif

#if __has_feature(assume_nonnull)
#define AG_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define AG_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")
#else
#define AG_ASSUME_NONNULL_BEGIN
#define AG_ASSUME_NONNULL_END
#endif

#if !__has_feature(nullability)
#ifndef _Nullable
#define _Nullable
#endif
#ifndef _Nonnull
#define _Nonnull
#endif
#ifndef _Null_unspecified
#define _Null_unspecified
#endif
#endif

#if __has_attribute(enum_extensibility)
#define __AG_ENUM_ATTRIBUTES __attribute__((enum_extensibility(open)))
#define __AG_CLOSED_ENUM_ATTRIBUTES __attribute__((enum_extensibility(closed)))
#define __AG_OPTIONS_ATTRIBUTES __attribute__((flag_enum,enum_extensibility(open)))
#else
#define __AG_ENUM_ATTRIBUTES
#define __AG_CLOSED_ENUM_ATTRIBUTES
#define __AG_OPTIONS_ATTRIBUTES
#endif

#define __AG_ENUM_FIXED_IS_AVAILABLE (__cplusplus && __cplusplus >= 201103L && (__has_extension(cxx_strong_enums) || __has_feature(objc_fixed_enum))) || (!__cplusplus && (__has_feature(objc_fixed_enum) || __has_extension(cxx_fixed_enum)))

#if __AG_ENUM_FIXED_IS_AVAILABLE
#define AG_ENUM(_type, _name)     enum __AG_ENUM_ATTRIBUTES _name : _type _name; enum _name : _type
#define AG_CLOSED_ENUM(_type, _name)      enum __AG_CLOSED_ENUM_ATTRIBUTES _name : _type _name; enum _name : _type
#if (__cplusplus)
#define AG_OPTIONS(_type, _name) __attribute__((availability(swift,unavailable))) _type _name; enum __AG_OPTIONS_ATTRIBUTES : _name
#else
#define AG_OPTIONS(_type, _name) enum __AG_OPTIONS_ATTRIBUTES _name : _type _name; enum _name : _type
#endif
#else
#define AG_ENUM(_type, _name) _type _name; enum
#define AG_CLOSED_ENUM(_type, _name) _type _name; enum
#define AG_OPTIONS(_type, _name) _type _name; enum
#endif

#if __has_attribute(swift_private)
#define AG_REFINED_FOR_SWIFT __attribute__((swift_private))
#else
#define AG_REFINED_FOR_SWIFT
#endif

#if __has_attribute(swift_name)
#define AG_SWIFT_NAME(_name) __attribute__((swift_name(#_name)))
#else
#define AG_SWIFT_NAME(_name)
#endif

#if __has_attribute(swift_wrapper)
#define AG_SWIFT_STRUCT __attribute__((swift_wrapper(struct)))
#else
#define AG_SWIFT_STRUCT
#endif

// Define mappings for calling conventions.

// Annotation for specifying a calling convention of
// a runtime function. It should be used with declarations
// of runtime functions like this:
// void runtime_function_name() AG_SWIFT_CC(swift)
#define AG_SWIFT_CC(CC) AG_SWIFT_CC_##CC

// AG_SWIFT_CC(c) is the C calling convention.
#define AG_SWIFT_CC_c

// AG_SWIFT_CC(swift) is the Swift calling convention.
#if __has_attribute(swiftcall)
#define AG_SWIFT_CC_swift __attribute__((swiftcall))
#define AG_SWIFT_CONTEXT __attribute__((swift_context))
#define AG_SWIFT_ERROR_RESULT __attribute__((swift_error_result))
#define AG_SWIFT_INDIRECT_RESULT __attribute__((swift_indirect_result))
#else
#define AG_SWIFT_CC_swift
#define AG_SWIFT_CONTEXT
#define AG_SWIFT_ERROR_RESULT
#define AG_SWIFT_INDIRECT_RESULT
#endif

#if __has_attribute(swift_attr)
#define AG_SWIFT_SHARED_REFERENCE(_retain, _release)                        \
  __attribute__((swift_attr("import_reference")))                           \
  __attribute__((swift_attr(_AG_STRINGIFY(retain:_retain))))       \
  __attribute__((swift_attr(_AG_STRINGIFY(release:_release))))
#else
#define AG_SWIFT_SHARED_REFERENCE(_retain, _release)
#endif

#if __has_include(<ptrcheck.h>)
#include <ptrcheck.h>
#define AG_COUNTED_BY(N) __counted_by(N)
#else
#define AG_COUNTED_BY(N)
#endif
