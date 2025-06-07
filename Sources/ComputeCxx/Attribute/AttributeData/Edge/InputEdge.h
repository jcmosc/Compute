#pragma once

#include "Attribute/AttributeID/AttributeID.h"

namespace AG {

struct InputEdge {
    enum class Flags : uint8_t {

    };

    AttributeID attribute;
    Flags flags;

    bool operator<(const InputEdge &other) const noexcept {
        return attribute != other.attribute ? attribute < other.attribute : flags < other.flags;
    }
};

} // namespace AG
