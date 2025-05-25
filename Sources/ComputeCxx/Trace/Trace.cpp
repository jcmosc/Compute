#include "Trace.h"

namespace AG {

void Trace::log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message_v(format, args);
    va_end(args);
}

} // namespace AG
