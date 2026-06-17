#pragma once

#include "Array/ArrayRef.h"
#include "Attribute/AttributeID/AttributeID.h"

namespace IAG {

struct OutputEdge {
    AttributeID attribute;
};

using ConstOutputEdgeArrayRef = ArrayRef<const OutputEdge>;

} // namespace IAG
