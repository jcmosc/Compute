#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID.h"
#include "Containers/ArrayRef.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class OutputEdge {
  public:
    AttributeID value;
};

using ConstOutputEdgeArrayRef = ArrayRef<const OutputEdge>;

} // namespace AG

CF_ASSUME_NONNULL_END
