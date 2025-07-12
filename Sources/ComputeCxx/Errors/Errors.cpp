#include <os/log.h>

#include "Graph/Graph.h"

char *error_message = nullptr;

namespace AG {

os_log_t error_log() {
    static os_log_t log = os_log_create("dev.incrematic.compute", "error");
    return log;
}

[[noreturn]] void precondition_failure(const char *format, ...) {
    char *message = nullptr;

    va_list args;
    va_start(args, format);
    vasprintf(&message, format, args);
    va_end(args);

    if (message != nullptr) {
        os_log_error(error_log(), "precondition failure: %s", message);
        Graph::trace_assertion_failure(true, "precondition failure: %s", message);
        if (error_message == nullptr) {
            asprintf(&error_message, "Compute precondition failure: %s.\n", message);
        }
        free(message);
    }

    abort();
}

void non_fatal_precondition_failure(const char *format, ...) {
    char *message = nullptr;

    va_list args;
    va_start(args, format);
    vasprintf(&message, format, args);
    va_end(args);

    if (message != nullptr) {
        os_log_fault(error_log(), "precondition failure: %s", message);
        free(message);
    }
}

} // namespace AG
