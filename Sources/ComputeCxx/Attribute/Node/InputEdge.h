#pragma once

#include "Data/Pointer.h"

namespace AG {

class Node;

struct InputEdge {
    struct Comparator {};

    data::ptr<Node> value;
    uint8_t flags;
};

} // namespace AG
