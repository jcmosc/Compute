#include "Log/Log.h"

namespace AG {

platform_log_t misc_log() {
    static platform_log_t log = platform_log_create("dev.incrematic.compute", "misc");
    return log;
}

} // namespace AG
