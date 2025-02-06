#pragma once

#include <CoreFoundation/CFBase.h>

#include "Array/ArrayRef.h"
#include "Attribute/AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class OutputEdge {
  public:
    AttributeID value;
};

using ConstOutputEdgeArrayRef = const ArrayRef<OutputEdge>;

} // namespace AG

CF_ASSUME_NONNULL_END
