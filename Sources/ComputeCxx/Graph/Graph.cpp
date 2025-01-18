#include "Graph.h"

#include "Attribute/AttributeType.h"
#include "Attribute/Node/Node.h"

namespace AG {

void Graph::trace_assertion_failure(bool all_stop_tracing, const char *format, ...) {
    // TODO: Not implemented
}

const AttributeType &Graph::attribute_type(uint32_t type_id) const {
    // TODO: Not implemented
    throw;
}

const AttributeType &Graph::attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const {
    auto &type = attribute_type(attribute->type_id());
    if (ref_out) {
        void *self = ((char *)attribute.get() + type.attribute_offset());
        if (attribute->has_indirect_self()) {
            self = *(void **)self;
        }
        *ref_out = self;
    }
    return type;
}

void Graph::did_allocate_node_value(size_t size) {
    // TODO: Not implemented
}

void Graph::did_destroy_node_value(size_t size) {
    // TODO: Not implemented
}

void Graph::update_attribute(AttributeID attribute, bool option) {
    // TODO: Not implemented
}

} // namespace AG
