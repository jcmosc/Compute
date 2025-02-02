#include "Graph.h"

#include <os/log.h>

#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/Node/Node.h"
#include "Attribute/OffsetAttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Errors/Errors.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Swift/Metadata.h"
#include "Trace.h"

namespace AG {

const AttributeType &Graph::attribute_type(uint32_t type_id) const { return *_types[type_id]; }

const AttributeType &Graph::attribute_ref(data::ptr<Node> node, const void *_Nullable *_Nullable ref_out) const {
    auto &type = attribute_type(node->type_id());
    if (ref_out) {
        void *self = ((char *)node.get() + type.attribute_offset());
        if (node->flags().has_indirect_self()) {
            self = *(void **)self;
        }
        *ref_out = self;
    }
    return type;
}

void Graph::attribute_modify(data::ptr<Node> node, const swift::metadata &metadata,
                             ClosureFunctionPV<void, void *> modify, bool flag) {
    if (!node->state().is_self_initialized()) {
        precondition_failure("no self data: %u", node);
    }

    auto type = attribute_type(node->type_id());
    if (&type.self_metadata() != &metadata) {
        precondition_failure("self type mismatch: %u", node);
    }

    foreach_trace([&node](Trace &trace) { trace.begin_modify(node); });

    void *body = (uint8_t *)node.get() + type.attribute_offset();
    if (node->flags().has_indirect_self()) {
        body = *(void **)body;
    }
    modify(body);

    foreach_trace([&node](Trace &trace) { trace.end_modify(node); });

    if (flag) {
        node->flags().set_value4_unknown0x40(true);
        mark_pending(node, node.get());
    }
}

// MARK: Attributes

data::ptr<Node> Graph::add_attribute(Subgraph &subgraph, uint32_t type_id, void *body, void *value) {
    const AttributeType &type = attribute_type(type_id);

    void *effective_value = nullptr;
    if (type.use_graph_as_initial_value()) {
        effective_value = this;
    }
    if (value != nullptr || type.value_metadata().vw_size() != 0) {
        effective_value = value;
    }

    void *buffer = nullptr;
    size_t size = type.self_metadata().vw_size();
    size_t alignment_mask = type.self_metadata().getValueWitnesses()->getAlignmentMask();
    if (!type.self_metadata().getValueWitnesses()->isBitwiseTakable() || type.self_metadata().vw_size() > 0x80) {
        size = sizeof(void *);
        alignment_mask = 7;
        buffer = subgraph.alloc_persistent(size);
    }

    size_t total_size = ((sizeof(Node) + alignment_mask) & ~alignment_mask) + size;

    data::ptr<Node> node;
    if (total_size > 0x10) {
        node = (data::ptr<Node>)subgraph.alloc_bytes_recycle(uint32_t(total_size), uint32_t(alignment_mask | 3));
    } else {
        node = (data::ptr<Node>)subgraph.alloc_bytes(uint32_t(total_size), uint32_t(alignment_mask | 3));
    }
    *node = Node(Node::State(type.node_initial_state()), type_id, 0x20);

    if (type_id >= 0x100000) {
        precondition_failure("too many node types allocated");
    }

    void *self = (uint8_t *)node.get() + type.attribute_offset();

    node->set_state(node->state().with_self_initialized(true));
    if (node->state().is_unknown3() && !type.value_metadata().getValueWitnesses()->isPOD()) {
        node->flags().set_value4_unknown0x20(!type.unknown_0x20()); // toggle
    } else {
        node->flags().set_value4_unknown0x20(false);
    }
    if (buffer != nullptr) {
        node->flags().set_has_indirect_self(true);
        *(void **)self = buffer;
    }

    if (!type.value_metadata().getValueWitnesses()->isBitwiseTakable() || type.value_metadata().vw_size() > 0x80) {
        node->flags().set_has_indirect_value(true);
    }

    _num_nodes += 1;
    _num_node_values += 1;

    if (type.self_metadata().vw_size() != 0) {
        void *self_dest = self;
        if (node->flags().has_indirect_self()) {
            self_dest = *(void **)self_dest;
        }
        type.self_metadata().vw_initializeWithCopy((swift::opaque_value *)self_dest, (swift::opaque_value *)body);
    }

    if (effective_value != nullptr) {
        value_set_internal(node, *node.get(), effective_value, type.value_metadata());
    } else {
        node->set_state(node->state().with_dirty(true).with_pending(true));
        subgraph.add_dirty_flags(node->flags().value3());
    }

    subgraph.add_node(node);
    return node;
}

void Graph::update_attribute(AttributeID attribute, bool option) {
    // TODO: Not implemented
}

#pragma mark - Indirect attributes

void Graph::add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset,
                                   std::optional<size_t> size, bool is_mutable) {
    if (subgraph.graph() != attribute.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    auto offset_attribute = attribute.resolve(AttributeID::TraversalOptions::SkipMutableReference);
    attribute = offset_attribute.attribute();
    if (__builtin_add_overflow(offset, offset_attribute.offset(), &offset) ||
        offset + offset_attribute.offset() > 0x3ffffffe) {
        precondition_failure("indirect attribute overflowed: %lu + %lu", offset, offset_attribute.offset());
    }

    if (size.has_value()) {
        auto attribute_size = attribute.size();
        if (attribute_size.has_value() && attribute_size.value() < offset + size.value()) {
            // TODO: check args
            precondition_failure("invalid size for indirect attribute: %d vs %u", attribute_size.has_value(),
                                 offset_attribute.offset());
        }
    }

    if (is_mutable) {
        auto indirect_node = (data::ptr<MutableIndirectNode>)subgraph.alloc_bytes(sizeof(MutableIndirectNode), 3);

        uint32_t zone_id = attribute.without_kind() != 0 ? attribute.subgraph()->info().zone_id() : 0;
        auto source = WeakAttributeID(attribute, zone_id);
        bool traversers_graph_contexts = subgraph.graph_context_id() != attribute.subgraph()->graph_context_id();
        uint16_t node_size = size.has_value() && size.value() <= 0xfffe ? uint16_t(size.value()) : 0xffff;
        *indirect_node = MutableIndirectNode(source, traversers_graph_contexts, offset, node_size, source, offset);

        add_input_dependencies(AttributeID(indirect_node).with_kind(AttributeID::Kind::Indirect), attribute);
        subgraph.add_indirect((data::ptr<IndirectNode>)indirect_node, true);
    } else {
        auto indirect_node = (data::ptr<IndirectNode>)subgraph.alloc_bytes_recycle(sizeof(Node), 3);

        uint32_t zone_id = attribute.without_kind() != 0 ? attribute.subgraph()->info().zone_id() : 0;
        auto source = WeakAttributeID(attribute, zone_id);
        bool traversers_graph_contexts = subgraph.graph_context_id() != attribute.subgraph()->graph_context_id();
        uint16_t node_size = size.has_value() && size.value() <= 0xfffe ? uint16_t(size.value()) : 0xffff;
        *indirect_node = IndirectNode(source, traversers_graph_contexts, offset, node_size);

        subgraph.add_indirect(indirect_node, &subgraph != attribute.subgraph());
    }
}

void Graph::indirect_attribute_set(data::ptr<IndirectNode> attribute, AttributeID source) {
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }
    if (AttributeID(attribute).subgraph()->graph() != source.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    foreach_trace([&attribute, &source](Trace &trace) { trace.set_source(attribute, source); });

    OffsetAttributeID resolved_source = source.resolve(AttributeID::TraversalOptions::SkipMutableReference);

    // TODO: ...
}

void Graph::indirect_attribute_reset(data::ptr<IndirectNode> attribute, bool flag) {}

const AttributeID &Graph::indirect_attribute_dependency(data::ptr<IndirectNode> attribute) {
    // Status: Verified
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }
    return attribute->to_mutable().dependency();
}

void Graph::indirect_attribute_set_dependency(data::ptr<IndirectNode> attribute, AttributeID dependency) {
    // Status: Verified
    if (dependency.without_kind()) {
        if (!dependency.is_direct()) {
            precondition_failure("indirect dependencies must be attributes");
        }
        if (AttributeID(attribute).subgraph() != dependency.subgraph()) {
            precondition_failure("indirect dependencies must share a subgraph with their attribute");
        }
    } else {
        dependency = AttributeID(0);
    }
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }

    foreach_trace([&attribute, &dependency](Trace &trace) { trace.set_dependency(attribute, dependency); });

    AttributeID old_dependency = attribute->to_mutable().dependency();
    if (old_dependency != dependency) {
        AttributeID indirect_attribute = AttributeID(attribute).with_kind(AttributeID::Kind::Indirect);
        if (old_dependency) {
            remove_output_edge((data::ptr<Node>)old_dependency, indirect_attribute);
        }
        attribute->to_mutable().set_dependency(dependency);
        if (dependency) {
            add_output_edge((data::ptr<MutableIndirectNode>)dependency, indirect_attribute);
            if (dependency.to_node().state().is_dirty()) {
                propagate_dirty(indirect_attribute);
            }
        }
    }
}

#pragma mark - Values

// Status: Verified
bool Graph::value_exists(data::ptr<Node> node) { return node->state().is_value_initialized(); }

AGValueState Graph::value_state(AttributeID attribute) {
    if (!attribute.is_direct()) {
        auto resolved_attribute = attribute.resolve(AttributeID::TraversalOptions::AssertNotNil);
        attribute = resolved_attribute.attribute();
    }
    if (!attribute.is_direct()) {
        return 0;
    }

    auto node = attribute.to_node();
    return (node.state().is_dirty() ? 1 : 0) << 0 | (node.state().is_pending() ? 1 : 0) << 1 |
           (node.state().is_evaluating() ? 1 : 0) << 2 | (node.state().is_value_initialized() ? 1 : 0) << 3 |
           (node.state().is_unknown2() ? 1 : 0) << 4 | (node.flags().value4_unknown0x20() ? 1 : 0) << 5 |
           (node.state().is_unknown3() ? 1 : 0) << 6 | (node.flags().value4_unknown0x40() ? 1 : 0) << 7;
}

void Graph::value_mark(data::ptr<Node> node) {
    //    iVar2 = tpidrro_el0;
    //    uVar6 = *(uint *)(iVar2 + _current_update_key * 8);
    //    if (((((uVar6 & 1) == 0) &&
    //          (puVar7 = (undefined8 *)(uVar6 & 0xfffffffffffffffe), puVar7 != (undefined8 *)0x0)) &&
    //         ((Graph *)*puVar7 == this)) && (((*node & 0xc0) != 0 || (0x1f < node[5])))) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("setting value during update: %u",(char)node_ptr,in_w2);
    //    }

    foreach_trace([&node](Trace &trace) { trace.mark_value(node); });

    AttributeType &type = *_types[node->type_id()];
    if (type.use_graph_as_initial_value()) {
        mark_changed(node, nullptr, nullptr, nullptr);
    } else {
        node->flags().set_value4_unknown0x40(true);

        if (!node->state().is_dirty()) {
            foreach_trace([&node](Trace &trace) { trace.set_dirty(node, true); });
            node->set_state(node->state().with_dirty(true));
        }
        if (!node->state().is_pending()) {
            foreach_trace([&node](Trace &trace) { trace.set_pending(node, true); });
            node->set_state(node->state().with_pending(true));
        }
        if (node->flags().value3()) {
            Subgraph *subgraph = AttributeID(node).subgraph();
            subgraph->add_dirty_flags(node->flags().value3());
        }
    }

    propagate_dirty(node);
}

void Graph::value_mark_all() {
    //    iVar8 = tpidrro_el0;
    //    uVar5 = *(uint *)(iVar8 + _current_update_key * 8);
    //    if (1 < uVar5 && (uVar5 & 1) == 0) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("invalidating all values during update",in_w1,in_w2);
    //    }

    for (auto subgraph : _subgraphs) {
        for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
            uint16_t relative_offset = page->relative_offset_1;
            while (relative_offset) {
                AttributeID attribute = AttributeID(page + relative_offset);
                if (attribute.is_nil()) {
                    break; // TODO: check if this should break out of entire loop
                }

                if (attribute.is_direct()) {
                    relative_offset = attribute.to_node().flags().relative_offset();
                } else if (attribute.is_indirect()) {
                    relative_offset = attribute.to_indirect_node().relative_offset();
                }

                if (attribute.is_direct()) {
                    auto node = attribute.to_node();
                    AttributeType &type = *_types[node.type_id()];
                    if (!type.use_graph_as_initial_value()) {
                        node.set_state(node.state().with_unknown3(true));
                        subgraph->add_dirty_flags(node.flags().value3());
                    }
                    node.foreach_input_edge([](InputEdge &edge) { edge.flags |= 8; });
                }
            }
        }
    }
}

bool Graph::value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value) {
    if (node->num_input_edges() > 0 && node->state().is_value_initialized()) {
        precondition_failure("can only set initial value of computed attributes: %u", node);
    }

    //    iVar1 = tpidrro_el0;
    //    uVar4 = *(uint *)(iVar1 + _current_update_key * 8);
    //    if (((((uVar4 & 1) == 0) &&
    //          (puVar5 = (undefined8 *)(uVar4 & 0xfffffffffffffffe), puVar5 != (undefined8 *)0x0)) &&
    //         ((Graph *)*puVar5 == this)) &&
    //        ((0x1f < node->child || ((node->lifecycle_flags & 0xc0U) != 0)))) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("setting value during update: %u",(char)attribute,(char)node);
    //    }

    bool changed = value_set_internal(node, *node.get(), value, value_type);
    if (changed) {
        propagate_dirty(node);
    }
    return changed;
}

bool Graph::value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value,
                               const swift::metadata &value_type) {
    foreach_trace([&node_ptr, &value](Trace &trace) { trace.set_value(node_ptr, value); });

    AttributeType &type = *_types[node.type_id()];
    if (&type.value_metadata() != &value_type) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)",
                             type.value_metadata().name(false), value_type.name(false));
    }

    if (node.state().is_value_initialized()) {
        // already initialized
        void *value_dest = node.get_value();
        if (node.flags().has_indirect_value()) {
            value_dest = *(void **)value_dest;
        }

        LayoutDescriptor::ComparisonOptions comparison_options =
            LayoutDescriptor::ComparisonOptions(type.comparison_mode()) |
            LayoutDescriptor::ComparisonOptions::CopyOnWrite | LayoutDescriptor::ComparisonOptions::ReportFailures;
        if (type.layout() == nullptr) {
            type.set_layout(LayoutDescriptor::fetch(value_type, comparison_options, 0));
        }
        ValueLayout layout = type.layout() == ValueLayoutEmpty ? nullptr : type.layout();

        // TODO: make void * and rename to dest and source
        if (LayoutDescriptor::compare(layout, (const unsigned char *)value_dest, (const unsigned char *)value,
                                      value_type.vw_size(), comparison_options)) {
            return false;
        }

        if (_traces.empty()) {
            // TODO: finish
        } else {
            mark_changed(AttributeID(node_ptr), type, value_dest, value, 0);
        }

        value_type.vw_assignWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
    } else {
        // not initialized yet
        node.allocate_value(*this, *AttributeID(node_ptr).subgraph());

        // TODO: wrap in initialize_value on Node...
        node.set_state(node.state().with_value_initialized(true));
        mark_changed(node_ptr, nullptr, nullptr, nullptr);

        void *value_dest = node.get_value();
        if (node.flags().has_indirect_value()) {
            value_dest = *(void **)value_dest;
        }
        value_type.vw_initializeWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
    }
}

// MARK: Marks

void Graph::mark_pending(data::ptr<Node> node_ptr, Node *node) {
    if (!node->state().is_pending()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_pending(node_ptr, true); });
        node->set_state(node->state().with_pending(true));
    }
    if (!node->state().is_dirty()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_dirty(node_ptr, true); });
        node->set_state(node->state().with_dirty(true));

        uint8_t dirty_flags = node->flags().value3();
        Subgraph *subgraph = AttributeID(node_ptr).subgraph();
        if (dirty_flags && subgraph != nullptr) {
            subgraph->add_dirty_flags(dirty_flags);
        }

        propagate_dirty(node_ptr);
    }
}

#pragma mark - Interning

uint32_t Graph::intern_key(const char *key) {
    if (_keys == nullptr) {
        _keys = new Graph::KeyTable(&_heap);
    }
    const char *found = nullptr;
    uint32_t key_id = _keys->lookup(key, &found);
    if (found == nullptr) {
        key_id = _keys->insert(key);
    }
    return key_id;
}

const char *Graph::key_name(uint32_t key_id) {
    if (_keys != nullptr && key_id < _keys->size()) {
        return _keys->get(key_id);
    }
    AG::precondition_failure("invalid string key id: %u", key_id);
}

uint32_t Graph::intern_type(swift::metadata *metadata, ClosureFunctionVP<void *> make_type) {
    uint32_t type_id = uint32_t(reinterpret_cast<uintptr_t>(_type_ids_by_metadata.lookup(metadata, nullptr)));
    if (type_id) {
        return type_id;
    }

    AttributeType *type = (AttributeType *)make_type();
    type->update_attribute_offset();

    static bool prefetch_layouts = []() -> bool {
        char *result = getenv("AG_PREFETCH_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    if (prefetch_layouts) {
        type->update_layout();
    }

    type_id = _types.size();
    if (type_id >= 0xffffff) {
        precondition_failure("overflowed max type id: %u", type_id);
    }
    _types.push_back(type);
    _type_ids_by_metadata.insert(metadata, reinterpret_cast<void *>(uintptr_t(type_id)));

    size_t self_size = type->self_metadata().vw_size();
    if (self_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute self: %u bytes, %s", uint(self_size),
                    type->self_metadata().name(false));
    }

    size_t value_size = type->value_metadata().vw_size();
    if (value_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute value: %u bytes, %s -> %s", uint(value_size),
                    type->self_metadata().name(false), type->value_metadata().name(false));
    }

    return type_id;
}

#pragma mark - Tracing

void Graph::add_trace(Trace *_Nullable trace) {
    if (trace == nullptr) {
        return;
    }
    trace->begin_trace(*this);
    _traces.push_back(trace);
}

void Graph::remove_trace(uint64_t trace_id) {
    auto iter = std::remove_if(_traces.begin(), _traces.end(),
                               [&trace_id](auto trace) -> bool { return trace->trace_id() == trace_id; });
    Trace *trace = *iter;
    trace->end_trace(*this);
    // destructor?
    _traces.erase(iter);
}

void Graph::trace_assertion_failure(bool all_stop_tracing, const char *format, ...) {
    // TODO: Not implemented
}

#pragma mark - Printing

void Graph::print_data() {
    data::table::shared().print();
    data::zone::print_header();
    for (auto subgraph : _subgraphs) {
        subgraph->data::zone::print(); // TODO: make first field..
    }
}

} // namespace AG
