#include <stdint.h>

#include "ComputeCxx/IAGBase.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

double current_time(void);
double absolute_time_to_seconds(uint64_t ticks);

} // namespace IAG

IAG_ASSUME_NONNULL_END
