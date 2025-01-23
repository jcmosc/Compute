#include "Time.h"

#include <mach/mach_time.h>
#include <math.h>

namespace AG {

double current_time() { return absolute_time_to_seconds(mach_absolute_time()); }

double absolute_time_to_seconds(uint64_t ticks) {
    static double time_scale = []() -> double {
        struct mach_timebase_info info;
        auto err = mach_timebase_info(&info);
        if (err) {
            return NAN;
        }
        return (info.numer / info.denom) * 0.000000001;
    }();
    return static_cast<double>(ticks) * time_scale;
}

} // namespace AG
