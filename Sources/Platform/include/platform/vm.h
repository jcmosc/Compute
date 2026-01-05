#pragma once

#if __APPLE__
#endif

#include <platform/base.h>

PLATFORM_ASSUME_NONNULL_BEGIN
PLATFORM_EXTERN_C_BEGIN

#if __APPLE__
#else
typedef uintptr_t   vm_offset_t;
typedef uintptr_t   vm_size_t;
typedef vm_offset_t vm_address_t;
#endif

PLATFORM_EXTERN_C_END
PLATFORM_ASSUME_NONNULL_END
