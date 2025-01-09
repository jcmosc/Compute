#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Attribute/AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType;

class Graph {
  public:
    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    const AttributeType &attribute_type(uint32_t type_id) const;
    const AttributeType &attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const;

    void did_allocate_node_value(size_t size);
    void did_destroy_node_value(size_t size);

    void update_attribute(AttributeID attribute, bool option);
};

} // namespace AG

CF_ASSUME_NONNULL_END
