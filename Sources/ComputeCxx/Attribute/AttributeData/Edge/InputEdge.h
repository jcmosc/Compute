#pragma once

#include "Attribute/AttributeID/AttributeID.h"
#include "ComputeCxx/AGInputOptions.h"

namespace AG {

struct InputEdge {
    AttributeID attribute;
    AGInputOptions options;

    struct Comparator {
        AttributeID attribute;
        AGInputOptions options_mask;
        AGInputOptions options;
        bool match(const InputEdge &input_edge) {
            return input_edge.attribute == attribute &&
                   (input_edge.options & options_mask) == options;
        }
    };

    bool operator<(const InputEdge &other) const noexcept {
        return attribute != other.attribute ? attribute < other.attribute
                                            : options < other.options;
    }
};

} // namespace AG
