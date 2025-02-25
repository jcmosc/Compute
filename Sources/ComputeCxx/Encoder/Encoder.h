#pragma once

#include <CoreFoundation/CFBase.h>

#include "Containers/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Encoder {
  public:
    struct Delegate {
        virtual int flush_encoder(Encoder &encoder){};
    };

  private:
    Delegate *_Nullable _delegate;
    uint64_t _flush_interval;
    void *_field_0x10;
    vector<char, 0, uint64_t> _buffer;
    vector<uint64_t, 0, uint64_t> _sections;

  public:
    Encoder(Delegate *_Nullable delegate, uint64_t flush_interval);

    const vector<char, 0, uint64_t> &buffer() const { return _buffer; };

    void encode_varint(uint64_t value);
    void encode_fixed64(uint64_t value);
    void encode_data(void *data, size_t length);

    void begin_length_delimited();
    void end_length_delimited();

    void flush();
};

} // namespace AG

CF_ASSUME_NONNULL_END
