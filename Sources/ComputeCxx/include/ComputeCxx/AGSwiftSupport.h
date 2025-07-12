#pragma once

#if __has_attribute(swift_private)
# define AG_REFINED_FOR_SWIFT __attribute__((swift_private))
#else
# define AG_REFINED_FOR_SWIFT
#endif


#if __has_attribute(swift_name)
# define AG_SWIFT_NAME(_name) __attribute__((swift_name(#_name)))
#else
# define AG_SWIFT_NAME(_name)
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

#if __has_attribute(swift_async_context)
#define AG_SWIFT_ASYNC_CONTEXT __attribute__((swift_async_context))
#else
#define AG_SWIFT_ASYNC_CONTEXT
#endif
