#pragma once

#include <cstring> 

namespace AG {
namespace LayoutDescriptor {

enum class ValueLayoutEntryKind : uint8_t {
    End = '\x00',
    Equals = '\x01',
    Indirect = '\x02',
    Existential = '\x03',
    HeapRef = '\x04',
    Function = '\x05',
    Nested = '\x06',
    CompactNested = '\x07',

    EnumStartVariadic = '\x08',
    EnumStart0 = '\x09',
    EnumStart1 = '\x0a',
    EnumStart2 = '\x0b',
    EnumContinueVariadic = '\x0c',
    EnumContinue0 = '\x0d',
    EnumContinue1 = '\x0e',
    EnumContinue2 = '\x0f',
    EnumContinue3 = '\x10',
    EnumContinue4 = '\x11',
    EnumContinue5 = '\x12',
    EnumContinue6 = '\x13',
    EnumContinue7 = '\x14',
    EnumContinue8 = '\x15',
    EnumEnd = '\x16',
};

struct ValueLayoutReader {
    const unsigned char *layout;

    inline ValueLayoutEntryKind &peek_kind() const { return *(ValueLayoutEntryKind *)layout; }
    
    inline ValueLayoutEntryKind read_kind() {
        auto kind = *(ValueLayoutEntryKind *)layout;
        layout += 1;
        return kind;
    }

    template <typename T> inline T read_bytes() {
        T returnVal;
        memcpy(&returnVal, layout, sizeof(T));
        layout += sizeof(T);
        return returnVal;
    }

    inline uint64_t read_varint() {
        unsigned shift = 0;
        uint64_t result = 0;
        while (*layout & 0x80) {
            result |= (*layout & 0x7f) << shift;
            shift += 7;
            layout += 1;
        }
        result |= (*layout & 0x7f) << shift;
        layout += 1;
        return result;
    }
    
    inline void skip_varint() {
        while (*layout & 0x80) {
            layout += 1;
        }
        layout += 1;
    }

    inline void skip(size_t n) { layout += n; }
};

} // namespace LayoutDescriptor
} // namespace AG
