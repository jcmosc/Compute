#include "Node.h"

#include "Attribute/AttributeType.h"
#include "Data/Pointer.h"
#include "Data/Zone.h"
#include "Graph/Graph.h"
#include "Swift/Metadata.h"

namespace AG {

void *Node::get_self(const AttributeType &type) {
    void *self = ((char *)this + type.attribute_offset());
    if (_flags.has_indirect_self()) {
        self = *(void **)self;
    }
    return self;
}

void Node::update_self(const Graph &graph, void *new_self) {
    auto type = graph.attribute_type(_info.type_id);
    void *self = get_self(type);

    if (!state().is_self_initialized()) {
        set_state(state().with_self_initialized(true));
        type.self_metadata().vw_initializeWithCopy(static_cast<swift::opaque_value *>(self),
                                                   static_cast<swift::opaque_value *>(new_self));
    } else {
        type.self_metadata().vw_assignWithCopy(static_cast<swift::opaque_value *>(self),
                                               static_cast<swift::opaque_value *>(new_self));
    }
}

void Node::destroy_self(const Graph &graph) {
    if (!state().is_self_initialized()) {
        return;
    }
    set_state(state().with_self_initialized(false));

    auto type = graph.attribute_type(_info.type_id);
    void *self = get_self(type);

    type.vt_destroy_self(self);
    type.self_metadata().vw_destroy(static_cast<swift::opaque_value *>(self));
}

void *Node::get_value() {
    void *value = _value.get();
    if (_flags.has_indirect_value()) {
        value = *(void **)value;
    }
    return value;
}

void Node::allocate_value(Graph &graph, data::zone &zone) {
    if (_value) {
        return;
    }

    auto type = graph.attribute_type(_info.type_id);
    size_t size = type.value_metadata().vw_size();
    size_t alignment = type.value_metadata().getValueWitnesses()->getAlignmentMask();

    if (_flags.has_indirect_value()) {
        _value = zone.alloc_bytes_recycle(sizeof(void *), sizeof(void *) - 1);
        void *value = zone.alloc_persistent(size);
        *(static_cast<data::ptr<void *>>(_value)).get() = value;
    } else {
        if (size <= 0x10) {
            _value = zone.alloc_bytes_recycle(uint32_t(size), uint32_t(alignment));
        } else {
            _value = zone.alloc_bytes(uint32_t(size), uint32_t(alignment));
        }
    }

    graph.did_allocate_node_value(size);
}

void Node::destroy_value(Graph &graph) {
    if (!state().is_value_initialized()) {
        return;
    }
    set_state(state().with_value_initialized(false));

    auto type = graph.attribute_type(_info.type_id);
    void *value = get_value();

    type.value_metadata().vw_destroy(static_cast<swift::opaque_value *>(value));
}

void Node::destroy(Graph &graph) {
    auto type = graph.attribute_type(_info.type_id);

    if (state().is_value_initialized()) {
        void *value = get_value();
        type.value_metadata().vw_destroy(static_cast<swift::opaque_value *>(value));
    }
    if (_value) {
        graph.did_destroy_node_value(type.value_metadata().vw_size());
    }

    if (state().is_self_initialized()) {
        void *self = get_self(type);

        type.vt_destroy_self(self);
        type.self_metadata().vw_destroy(static_cast<swift::opaque_value *>(self));
    }
}

} // namespace AG
