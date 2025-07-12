#pragma once

#include "Attribute/AttributeID/AttributeID.h"
#include "ComputeCxx/AGInputOptions.h"

namespace AG {

struct InputEdge {
    AttributeID attribute;
    AGInputOptions options;

    bool operator<(const InputEdge &other) const noexcept {
        return attribute != other.attribute ? attribute < other.attribute : options < other.options;
    }
};

} // namespace AG
