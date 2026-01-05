#pragma once

#if __APPLE__

#include <mach/mach_time.h>

typedef dispatch_time_t platform_time_t;

#define platform_absolute_time() mach_absolute_time()

#else

#include <time.h>
#include <platform/base.h>

typedef uint64_t platform_time_t;

#define platform_absolute_time() ({ \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    (uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1000000000UL; \
})

#endif

//PLATFORM_INLINE
//uint64_t platform_absolute_time() {
//#if TARGET_OS_MAC
//    
//    ULONGLONG ullTime;
//    QueryUnbiasedInterruptTimePrecise(&ullTime);
//    return ullTime;
//#elif TARGET_OS_LINUX || TARGET_OS_BSD || TARGET_OS_MAC || TARGET_OS_WASI

//#endif
//}
