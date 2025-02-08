#pragma once

#include "Attribute/AttributeID.h"
#include "Containers/ArrayRef.h"

namespace AG {

class Node;

struct InputEdge {
    struct Comparator {};
    enum Flags : uint8_t {
        Unknown2 = 1 << 2,
        IsPending = 1 << 3,
        Unknown4 = 1 << 4,
    };

    AttributeID value;
    uint8_t _flags;
    
    bool is_unknown2() { return _flags & Flags::Unknown2; };

    bool is_pending() { return _flags & Flags::IsPending; };
    void set_pending(bool value) { _flags = (_flags & ~Flags::IsPending) | (value ? Flags::IsPending : 0); };
    
    bool is_unknown4() { return _flags & Flags::Unknown4; };
    void set_unknown4(bool value) { _flags = (_flags & ~Flags::Unknown4) | (value ? Flags::Unknown4 : 0); };
};

using ConstInputEdgeArrayRef = const ArrayRef<InputEdge>;

struct OutputEdge {
    AttributeID value;
};

using ConstOutputEdgeArrayRef = ArrayRef<const OutputEdge>;

} // namespace AG
