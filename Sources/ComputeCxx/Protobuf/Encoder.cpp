#include "Encoder.h"

#include "Errors/Errors.h"

namespace IAG {

Encoder::Encoder(Delegate *_Nullable delegate, uint64_t flush_interval)
    : _delegate(delegate), _flush_interval(flush_interval) {
    if (delegate == nullptr && flush_interval != 0) {
        precondition_failure("need a delegate if flush interval is non-zero");
    }
}

void Encoder::encode_varint(uint64_t value) {
    uint64_t width = 0;
    if (value <= 0x7f) {
        if (_buffer.size() < _buffer.capacity()) {
            _buffer.push_back(value);
            return;
        }
        width = 1;
    } else {
        width = ((64 - std::countl_zero(value)) + 6) / 7;
    }

    uint64_t index = _buffer.size();
    _buffer.resize(_buffer.size() + width); // TODO: how to resize without zeroing memory

    _buffer[index] = 0;
    uint64_t remaining_value = value;
    while (remaining_value) {
        _buffer[index] = ((char)remaining_value & 0x7f) | (0x7f < remaining_value) << 7;
        index += 1;
        remaining_value = remaining_value >> 7;
    }
}

void Encoder::encode_fixed64(uint64_t value) {
    uint64_t old_size = _buffer.size();
    _buffer.resize(_buffer.size() + sizeof(uint64_t));
    // Protobuf fixed64 is little-endian
    char *dest = _buffer.data() + old_size;
    for (int i = 0; i < 8; ++i) {
        dest[i] = (char)(value >> (i * 8));
    }
}

void Encoder::encode_data(const void *data, size_t length) {
    encode_varint(length);
    if (length == 0) {
        return;
    }
    uint64_t old_size = _buffer.size();
    _buffer.resize(_buffer.size() + length);
    std::memcpy(_buffer.data() + old_size, data, length);
}

void Encoder::begin_length_delimited() {
    // Reserve one byte for the length and store the position
    uint64_t position = _buffer.size();
    _buffer.resize(_buffer.size() + 1);
    _sections.push_back(position);
}

void Encoder::end_length_delimited() {
    assert(!_sections.empty());

    uint64_t position = _sections.back();
    _sections.pop_back();

    uint64_t length = _buffer.size() - (position + 1);
    if (length <= 0x7f) {
        _buffer[position] = length;
    } else {
        // The length requires more than one byte
        uint64_t width = ((64 - std::countl_zero(length)) + 6) / 7;
        _buffer.resize(_buffer.size() + width - 1);

        std::memmove(reinterpret_cast<void *>(_buffer.data() + position + width),
                     reinterpret_cast<void *>(_buffer.data() + position + 1), length);

        uint64_t remaining_value = length;
        while (remaining_value) {
            _buffer[position] = ((char)remaining_value & 0x7f) | (0x7f < remaining_value) << 7;
            position += 1;
            remaining_value = remaining_value >> 7;
        }
    }

    if (_sections.empty() && _flush_interval != 0 && _flush_interval <= _buffer.size()) {
        flush();
    }
}

void Encoder::flush() {
    assert(_sections.empty());
    if (!_buffer.empty() && _delegate) {
        if (_delegate->flush_encoder(*this) == 0) {
            _buffer.resize(0);
        }
    }
}

} // namespace IAG
