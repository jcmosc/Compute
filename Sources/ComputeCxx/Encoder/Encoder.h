#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Encoder {
  public:
    void encode_varint(uint64_t value);

    void begin_length_delimited();
    void end_length_delimited();
};

} // namespace AG

CF_ASSUME_NONNULL_END
