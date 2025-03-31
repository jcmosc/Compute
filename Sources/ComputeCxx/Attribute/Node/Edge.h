#pragma once

#include "Attribute/AttributeID.h"
#include "Containers/ArrayRef.h"

namespace AG {

class Node;

struct InputEdge {
    enum Flags : uint8_t {
        Unprefetched = 1 << 0,
        Unknown1 = 1 << 1,
        AlwaysEnabled = 1 << 2,
        Changed = 1 << 3, // set when node is dirty
        Unknown4 = 1 << 4,
    };
    
    struct Comparator {
        AttributeID attribute;
        uint8_t flags_mask;
        uint8_t flags;
        bool match(InputEdge &input) { return input.value == attribute && (input._flags & flags_mask) == flags; }
    };

    AttributeID value;
    uint8_t _flags;
    
    bool is_unprefetched() { return _flags & Flags::Unprefetched; };
    void set_unprefetched(bool value) { _flags = (_flags & ~Flags::Unprefetched) | (value ? Flags::Unprefetched : 0); };

    bool is_unknown1() { return _flags & Flags::Unknown1; };
    void set_unknown1(bool value) { _flags = (_flags & ~Flags::Unknown1) | (value ? Flags::Unknown1 : 0); };
    
    bool is_always_enabled() { return _flags & Flags::AlwaysEnabled; };
    void set_always_enabled(bool value) { _flags = (_flags & ~Flags::AlwaysEnabled) | (value ? Flags::AlwaysEnabled : 0); };

    bool is_changed() { return _flags & Flags::Changed; };
    void set_changed(bool value) { _flags = (_flags & ~Flags::Changed) | (value ? Flags::Changed : 0); };

    bool is_unknown4() { return _flags & Flags::Unknown4; };
    void set_unknown4(bool value) { _flags = (_flags & ~Flags::Unknown4) | (value ? Flags::Unknown4 : 0); };

    bool operator<(const InputEdge &other) const noexcept {
        return value != other.value ? value < other.value : _flags < other._flags;
    };
};

using ConstInputEdgeArrayRef = const ArrayRef<InputEdge>;

struct OutputEdge {
    AttributeID value;
};

using ConstOutputEdgeArrayRef = ArrayRef<const OutputEdge>;

} // namespace AG
