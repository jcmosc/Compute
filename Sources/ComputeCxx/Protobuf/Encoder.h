#pragma once

#include "ComputeCxx/IAGBase.h"

#include "Vector/Vector.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class Encoder {
  public:
    struct Delegate {
        virtual int flush_encoder(Encoder &encoder) { return 0; };
    };

  private:
    Delegate *_Nullable _delegate;
    uint64_t _flush_interval;
    vector<char, 0, uint64_t> _buffer;
    vector<uint64_t, 0, uint64_t> _sections;

    void encode_varint(uint64_t value);
    void encode_fixed64(uint64_t value);
    void encode_data(const void *data, size_t length);

    void begin_length_delimited();
    void end_length_delimited();

    enum class WireType : uint8_t {
        VarInt = 0,
        I64 = 1,
        Len = 2,
    };

    void encode_tag(uint64_t field, WireType wire_type) {
        encode_varint((field << 3) | static_cast<uint8_t>(wire_type));
    }

  public:
    Encoder(Delegate *_Nullable delegate, uint64_t flush_interval);

    const vector<char, 0, uint64_t> &buffer() const { return _buffer; };

    void encode_field_varint(uint64_t field, uint64_t value) {
        if (value) {
            encode_tag(field, WireType::VarInt);
            encode_varint(value);
        }
    }

    void encode_field_fixed64(uint64_t field, uint64_t value) {
        if (value) {
            encode_tag(field, WireType::I64);
            encode_fixed64(value);
        }
    }

    void encode_field_data(uint64_t field, const void *data, size_t length) {
        if (length > 0) {
            encode_tag(field, WireType::Len);
            encode_data(data, length);
        }
    }

    void encode_field_begin(uint64_t field) {
        encode_tag(field, WireType::Len);
        begin_length_delimited();
    }

    void encode_field_end() { end_length_delimited(); }

    void flush();
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
