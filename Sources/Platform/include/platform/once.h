#pragma once

#if __APPLE__
#include <dispatch/dispatch.h>
#else
#include <pthread.h>
#endif

#include <platform/base.h>

PLATFORM_ASSUME_NONNULL_BEGIN
PLATFORM_EXTERN_C_BEGIN

#if __APPLE__

typedef dispatch_once_t platform_once_t;
typedef void (*platform_once_function_t)(void);

PLATFORM_INLINE
void _platform_once_thunk(void *_Nullable context) {
    ((dispatch_function_t)context)(NULL);
}

PLATFORM_INLINE
void platform_once(platform_once_t *predicate, platform_once_function_t function) {
    dispatch_once_f(predicate, (void *)function, _platform_once_thunk);
}

#else

typedef pthread_once_t platform_once_t;
typedef void (*platform_once_function_t)(void);

PLATFORM_INLINE
void platform_once(platform_once_t *predicate, platform_once_function_t function) {
    pthread_once(predicate, function);
}

#endif

PLATFORM_EXTERN_C_END
PLATFORM_ASSUME_NONNULL_END
