#include "Node.h"

#include "Attribute/AttributeType.h"
#include "Data/Pointer.h"
#include "Data/Zone.h"
#include "Graph/Graph.h"
#include "Swift/Metadata.h"

namespace AG {

void Node::update_self(const Graph &graph, void *new_self) {
    auto type = graph.attribute_type(_type_id);
    void *self = ((char *)this + type.attribute_offset());
    if (has_indirect_self()) {
        self = *(void **)self;
    }

    if (!_state.is_self_initialized()) {
        _state = _state.with_self_initialized(true);
        type.self_metadata().vw_initializeWithCopy(static_cast<swift::opaque_value *>(self),
                                                   static_cast<swift::opaque_value *>(new_self));
    } else {
        type.self_metadata().vw_assignWithCopy(static_cast<swift::opaque_value *>(self),
                                               static_cast<swift::opaque_value *>(new_self));
    }
}

void Node::destroy_self(const Graph &graph) {
    if (!_state.is_self_initialized()) {
        return;
    }
    _state = _state.with_self_initialized(false);

    auto type = graph.attribute_type(_type_id);
    void *self = ((char *)this + type.attribute_offset());
    if (has_indirect_self()) {
        self = *(void **)self;
    }

    type.v_destroy_self(self);
    type.self_metadata().vw_destroy(static_cast<swift::opaque_value *>(self));
}

void Node::allocate_value(Graph &graph, data::zone &zone) {
    if (_value) {
        return;
    }

    auto type = graph.attribute_type(_type_id);
    size_t size = type.value_metadata().vw_size();
    size_t alignment = type.value_metadata().vw_alignment();

    if (has_indirect_value()) {
        _value = zone.alloc_bytes_recycle(sizeof(void *), sizeof(void *) - 1);
        void *persistent_buffer = zone.alloc_persistent(size);
        *_value.unsafe_cast<void *>().get() = persistent_buffer;
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
    if (!_state.is_value_initialized()) {
        return;
    }
    _state = _state.with_value_initialized(false);

    auto type = graph.attribute_type(_type_id);
    void *value = _value.get();
    if (has_indirect_value()) {
        value = *(void **)value;
    }

    type.value_metadata().vw_destroy(static_cast<swift::opaque_value *>(value));
}

void Node::destroy(Graph &graph) {
    auto type = graph.attribute_type(_type_id);

    if (_state.is_value_initialized()) {
        void *value = _value.get();
        if (has_indirect_value()) {
            value = *(void **)value;
        }
        type.value_metadata().vw_destroy(static_cast<swift::opaque_value *>(value));
    }
    if (_value) {
        graph.did_destroy_node_value(type.value_metadata().vw_size());
    }

    if (_state.is_self_initialized()) {
        void *self = ((char *)this + type.attribute_offset());
        if (has_indirect_self()) {
            self = *(void **)self;
        }

        type.v_destroy_self(self);
        type.self_metadata().vw_destroy(static_cast<swift::opaque_value *>(self));
    }
}

} // namespace AG
