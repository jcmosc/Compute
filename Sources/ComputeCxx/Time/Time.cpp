#include "Time.h"

#if TARGET_OS_MAC
#include <mach/mach_time.h>
#else
#include <time.h>
#endif
#include <math.h>

namespace AG {

double current_time() {
#if TARGET_OS_MAC
    uint64_t ticks = mach_absolute_time();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return NAN;
    }

    // Convert to nanoseconds to match Darwin "ticks" semantics
    uint64_t ticks = static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
#endif
    return absolute_time_to_seconds(ticks);
}

double absolute_time_to_seconds(uint64_t ticks) {
#if TARGET_OS_MAC
    static double time_scale = []() -> double {
        struct mach_timebase_info info;
        auto err = mach_timebase_info(&info);
        if (err) {
            return NAN;
        }
        return (info.numer / info.denom) * 1e-9;
    }();
#else
    // On POSIX, ticks are already nanoseconds
    static const double time_scale = 1e-9;
#endif
    return static_cast<double>(ticks) * time_scale;
}

} // namespace AG
