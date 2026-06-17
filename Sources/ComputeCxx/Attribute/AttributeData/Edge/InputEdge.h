#pragma once

#include "Attribute/AttributeID/AttributeID.h"
#include "ComputeCxx/IAGInputOptions.h"

namespace IAG {

struct InputEdge {
    AttributeID attribute;
    IAGInputOptions options;

    struct Comparator {
        AttributeID attribute;
        IAGInputOptions options_mask;
        IAGInputOptions options;
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

} // namespace IAG
