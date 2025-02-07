#pragma once

#include "Attribute/AttributeID.h"
#include "Containers/ArrayRef.h"

namespace AG {

class Node;

struct InputEdge {
    struct Comparator {};

    AttributeID value;
    uint8_t flags;
};

using ConstInputEdgeArrayRef = const ArrayRef<InputEdge>;

} // namespace AG
