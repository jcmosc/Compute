#include "Encoder.h"

#include "Errors/Errors.h"

namespace AG {

Encoder::Encoder(Delegate *_Nullable delegate, uint64_t flush_interval)
    : _delegate(delegate), _flush_interval(flush_interval) {
    if (delegate == nullptr && flush_interval != 0) {
        precondition_failure("need a delegate if flush interval is non-zero");
    }
}

void Encoder::encode_varint(uint64_t value) {
    uint64_t width = 0;
    if (value < 0x80) {
        if (_buffer.capacity() > _buffer.size()) {
            _buffer.push_back((const char)value);
            return;
        }
        width = 1;
    } else {
        width = (63 - std::countl_zero(value)) / 7;
    }

    uint64_t index = _buffer.size();
    _buffer.resize(_buffer.size() + width); // TODO: how to resize without zeroing memory

    uint64_t remaining_value = value;
    while (remaining_value) {
        _buffer[index] = ((char)remaining_value & 0x7f) | (0x7f < remaining_value) << 7;
        index += 1;
        remaining_value = remaining_value >> 7;
    };
}

void Encoder::encode_fixed64(uint64_t value) {
    uint64_t *pointer = (uint64_t *)(_buffer.data() + _buffer.size());
    _buffer.resize(_buffer.size() + sizeof(uint64_t));
    *pointer = value;
}

void Encoder::encode_data(void *data, size_t length) {
    encode_varint(length);
    if (length == 0) {
        return;
    }
    void *pointer = (void *)(_buffer.data() + _buffer.size());
    _buffer.resize(_buffer.size() + length);
    memcpy(pointer, data, length);
}

void Encoder::begin_length_delimited() {
    // Reserve one byte for the length and store the position
    uint64_t old_length = _buffer.size();
    _buffer.resize(_buffer.size() + 1);
    _sections.push_back(old_length);
}

void Encoder::end_length_delimited() {
    uint64_t index = _sections.back();
    _sections.pop_back();

    uint64_t length = _buffer.size() - (index + 1);
    if (length < 0x80) {
        _buffer[index] = length;
    } else {
        // The length requires more than one byte
        uint64_t width = (63 - std::countl_zero(length)) / 7;
        _buffer.resize(_buffer.size() + width - 1);

        memmove((void *)(_buffer.data() + index + width), (void *)(_buffer.data() + index + 1), length);

        uint64_t remaining_value = length;
        while (remaining_value) {
            _buffer[index] = ((char)remaining_value & 0x7f) | (0x7f < remaining_value) << 7;
            index += 1;
            remaining_value = remaining_value >> 7;
        };
    }

    if (_sections.empty() && _flush_interval != 0 && _flush_interval <= _buffer.size()) {
        flush();
    }
}

void Encoder::flush() {
    if (!_buffer.empty() && _delegate) {
        _delegate->flush_encoder(*this);
        _buffer.resize(0);
    }
}

} // namespace AG
