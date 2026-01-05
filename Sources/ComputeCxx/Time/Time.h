#include <stdint.h>

#include "ComputeCxx/AGBase.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

double current_time(void);
double absolute_time_to_seconds(uint64_t ticks);

}

AG_ASSUME_NONNULL_END
