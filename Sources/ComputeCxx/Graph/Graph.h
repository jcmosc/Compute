#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType;

class Graph {
  public:
    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    const AttributeType &type_for_attribute(const Node &node) const;

    void update_attribute(AttributeID attribute, bool option);
};

} // namespace AG

CF_ASSUME_NONNULL_END
