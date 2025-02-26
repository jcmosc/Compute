#include "ExternalTrace.h"

#include "Graph/Context.h"
#include "Graph/Graph.h"
#include "Layout/AGComparison-Private.h"

void ExternalTrace::begin_trace(const AG::Graph &graph) {
    auto cf_graph = graph.main_context()->to_cf();
    if (auto callback = _interface.begin_trace) {
        callback(_trace, cf_graph);
    }
}

void ExternalTrace::end_trace(const AG::Graph &graph) {
    auto cf_graph = graph.main_context()->to_cf();
    if (auto callback = _interface.end_trace) {
        callback(_trace, cf_graph);
    }
}

void ExternalTrace::begin_update(const AG::Subgraph &subgraph, uint32_t options) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _interface.begin_update_subgraph) {
        callback(_trace, cf_subgraph);
    }
}

void ExternalTrace::end_update(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _interface.end_update_subgraph) {
        callback(_trace, cf_subgraph);
    }
}

void ExternalTrace::begin_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                                 uint32_t options) {
    if (auto callback = _interface.begin_update_stack) {
        callback(_trace, node);
    }
}

void ExternalTrace::end_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                               AG::Graph::UpdateStatus update_status) {
    if (auto callback = _interface.end_update_stack) {
        callback(_trace, update_status == AG::Graph::UpdateStatus::Changed);
    }
}

void ExternalTrace::begin_update(AG::data::ptr<AG::Node> node) {
    if (auto callback = _interface.begin_update_node) {
        callback(_trace);
    }
}

void ExternalTrace::end_update(AG::data::ptr<AG::Node> node, bool changed) {
    if (auto callback = _interface.end_update_node) {
        callback(_trace);
    }
}

void ExternalTrace::begin_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.begin_update_context) {
        callback(_trace, cf_context);
    }
}

void ExternalTrace::end_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.end_update_context) {
        callback(_trace, cf_context);
    }
}

void ExternalTrace::begin_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.begin_invalidation) {
        callback(_trace, cf_context, attribute);
    }
}

void ExternalTrace::end_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.end_invalidation) {
        callback(_trace, cf_context, attribute);
    }
}

void ExternalTrace::begin_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _interface.begin_modify) {
        callback(_trace);
    }
}

void ExternalTrace::end_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _interface.end_modify) {
        callback(_trace);
    }
}

void ExternalTrace::begin_event(AG::data::ptr<AG::Node> node, uint32_t event_id) {
    if (auto callback = _interface.begin_event) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_trace, node, event_name);
        }
    }
}

void ExternalTrace::end_event(AG::data::ptr<AG::Node> node, uint32_t event_id) {
    if (auto callback = _interface.end_event) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_trace, node, event_name);
        }
    }
}

void ExternalTrace::created(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.created_context) {
        callback(_trace, cf_context);
    }
}

void ExternalTrace::destroy(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.destroy_context) {
        callback(_trace, cf_context);
    }
}

void ExternalTrace::needs_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _interface.needs_update_context) {
        callback(_trace, cf_context);
    }
}

void ExternalTrace::created(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _interface.created_subgraph) {
        callback(_trace, cf_subgraph);
    }
}

void ExternalTrace::invalidate(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _interface.invalidate_subgraph) {
        callback(_trace, cf_subgraph);
    }
}

void ExternalTrace::destroy(const AG::Subgraph &subgraph) {
    // no method in binary
}

void ExternalTrace::add_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _interface.add_child_subgraph) {
        callback(_trace, cf_subgraph, cf_child);
    }
}

void ExternalTrace::remove_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _interface.remove_child_subgraph) {
        callback(_trace, cf_subgraph, cf_child);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::Node> node) {
    if (auto callback = _interface.added_node) {
        callback(_trace);
    }
}

void ExternalTrace::add_edge(AG::data::ptr<AG::Node> node, AG::AttributeID input, uint8_t input_edge_flags) {
    if (auto callback = _interface.add_edge) {
        callback(_trace);
    }
}

void ExternalTrace::remove_edge(AG::data::ptr<AG::Node> node, uint32_t input_index) {
    if (auto callback = _interface.remove_edge) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_trace);
        }
    }
}

void ExternalTrace::set_edge_pending(AG::data::ptr<AG::Node> node, uint32_t input_index, bool pending) {
    if (auto callback = _interface.set_edge_pending) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_trace);
        }
    }
}

void ExternalTrace::set_dirty(AG::data::ptr<AG::Node> node, bool dirty) {
    if (auto callback = _interface.set_dirty) {
        callback(_trace);
    }
}

void ExternalTrace::set_pending(AG::data::ptr<AG::Node> node, bool pending) {
    if (auto callback = _interface.set_pending) {
        callback(_trace);
    }
}

void ExternalTrace::set_value(AG::data::ptr<AG::Node> node, const void *value) {
    if (auto callback = _interface.set_value) {
        callback(_trace);
    }
}

void ExternalTrace::mark_value(AG::data::ptr<AG::Node> node) {
    if (auto callback = _interface.mark_value) {
        callback(_trace);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::IndirectNode> indirect_node) {
    if (auto callback = _interface.added_indirect_node) {
        callback(_trace, AG::AttributeID(indirect_node)); // TODO: check sets kind
    }
}

void ExternalTrace::set_source(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID source) {
    if (auto callback = _interface.set_source) {
        callback(_trace, AG::AttributeID(indirect_node)); // TODO: check sets kind
    }
}

void ExternalTrace::set_dependency(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID dependency) {
    if (auto callback = _interface.set_dependency) {
        callback(_trace, AG::AttributeID(indirect_node)); // TODO: check sets kind
    }
}

void ExternalTrace::mark_profile(const AG::Graph &graph, uint32_t event_id) {
    if (auto callback = _interface.mark_profile) {
        const char *event_name = graph.key_name(event_id);
        callback(_trace, event_name);
    }
}

// void ExternalTrace::destroy(const AG::Subgraph &subgraph) {
//     auto cf_subgraph = subgraph.to_cf();
//     if (auto callback = _interface.destroy_subgraph) {
//         callback(_trace, cf_subgraph);
//     }
// }

void ExternalTrace::custom_event(const AG::Graph::Context &context, const char *event_name, const void *value,
                                 const AG::swift::metadata &type) {
    if (_interface.options != 0) {
        auto cf_context = context.to_cf();
        if (auto callback = _interface.custom_event) {
            callback(_trace, cf_context, event_name, value, AGTypeID(&type));
        }
    }
}

void ExternalTrace::named_event(const AG::Graph::Context &context, uint32_t arg2, uint32_t num_args,
                                const uint64_t *event_args, CFDataRef data, uint32_t arg6) {
    if (_interface.options > 1) {
        auto cf_context = context.to_cf();
        if (auto callback = _interface.named_event) {
            callback(_trace, cf_context, arg2, num_args, event_args, data, arg6);
        }
    }
}

bool ExternalTrace::named_event_enabled(uint32_t event_id) {
    if (_interface.options < 2) {
        return false;
    }
    if (auto callback = _interface.named_event_enabled) {
        return callback(_trace);
    }
    return _interface.named_event != nullptr;
}

void ExternalTrace::set_deadline(uint64_t deadline) {
    if (_interface.options > 2) {
        if (auto callback = _interface.set_deadline) {
            callback(_trace);
        }
    }
}

void ExternalTrace::passed_deadline() {
    if (_interface.options > 2) {
        if (auto callback = _interface.passed_deadline) {
            callback(_trace);
        }
    }
}

void ExternalTrace::compare_failed(AG::data::ptr<AG::Node> node, const void *lhs, const void *rhs, size_t range_offset,
                                   size_t range_size, const AG::swift::metadata *_Nullable type) {
    if (_interface.options > 3) {
        AGComparisonStateStorage storage = {lhs, rhs, range_offset, range_size, AGTypeID(&type)};
        if (auto callback = _interface.compare_failed) {
            callback(_trace, node, &storage);
        }
    }
}
