#include <os/log.h>

#include "Graph/Graph.h"

namespace AG {

os_log_t misc_log() {
    static os_log_t log = os_log_create("dev.incrematic.compute", "misc");
    return log;
}

} // namespace AG
