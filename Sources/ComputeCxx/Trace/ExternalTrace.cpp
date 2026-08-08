#include "ExternalTrace.h"

#include "Comparison/IAGComparison-Private.h"
#include "ComputeCxx/IAGGraph.h"
#include "Graph/Context.h"
#include "Graph/Graph.h"

void ExternalTrace::begin_trace(const IAG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->begin_trace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::end_trace(const IAG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->end_trace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::begin_update(const IAG::Subgraph &subgraph, IAGAttributeFlags subgraph_flags) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto callback = _trace->begin_subgraph_update) {
            callback(_context, cf_subgraph, subgraph_flags);
        }
    }
}

void ExternalTrace::end_update(const IAG::Subgraph &subgraph) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto callback = _trace->end_subgraph_update) {
            callback(_context, cf_subgraph);
        }
    }
}

void ExternalTrace::begin_update(const IAG::Graph::UpdateStack &update_stack, IAG::data::ptr<IAG::Node> node,
                                 uint32_t options) {
    if (auto callback = _trace->begin_node_update) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::end_update(const IAG::Graph::UpdateStack &update_stack, IAG::data::ptr<IAG::Node> node,
                               IAGGraphUpdateStatus update_status) {
    if (auto callback = _trace->end_node_update) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), update_status == IAGGraphUpdateStatusChanged);
    }
}

void ExternalTrace::begin_update(IAG::data::ptr<IAG::Node> node) {
    if (auto callback = _trace->begin_value_update) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::end_update(IAG::data::ptr<IAG::Node> node, bool changed) {
    if (auto callback = _trace->end_value_update) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), changed);
    }
}

void ExternalTrace::begin_update(const IAG::Graph::Context &context) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->begin_graph_update) {
            callback(_context, cf_context);
        }
    }
}

void ExternalTrace::end_update(const IAG::Graph::Context &context) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->end_graph_update) {
            callback(_context, cf_context);
        }
    }
}

void ExternalTrace::begin_invalidation(const IAG::Graph::Context &context, IAG::AttributeID attribute) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->begin_graph_invalidation) {
            callback(_context, cf_context, IAGAttribute(attribute));
        }
    }
}

void ExternalTrace::end_invalidation(const IAG::Graph::Context &context, IAG::AttributeID attribute) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->end_graph_invalidation) {
            callback(_context, cf_context, IAGAttribute(attribute));
        }
    }
}

void ExternalTrace::begin_modify(IAG::data::ptr<IAG::Node> node) {
    if (auto callback = _trace->begin_modify_node) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::end_modify(IAG::data::ptr<IAG::Node> node) {
    if (auto callback = _trace->end_modify_node) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::begin_event(IAG::data::ptr<IAG::Node> node, uint32_t event_id) {
    if (auto callback = _trace->begin_event) {
        if (auto subgraph = IAG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, IAGAttribute(IAG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::end_event(IAG::data::ptr<IAG::Node> node, uint32_t event_id) {
    if (auto callback = _trace->end_event) {
        if (auto subgraph = IAG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, IAGAttribute(IAG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::created(const IAG::Graph::Context &context) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->graph_created) {
            callback(_context, cf_context);
        }
    }
}

void ExternalTrace::destroy(const IAG::Graph::Context &context) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->graph_destroy) {
            callback(_context, cf_context);
        }
    }
}

void ExternalTrace::needs_update(const IAG::Graph::Context &context) {
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->graph_needs_update) {
            callback(_context, cf_context);
        }
    }
}

void ExternalTrace::created(const IAG::Subgraph &subgraph) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto callback = _trace->subgraph_created) {
            callback(_context, cf_subgraph);
        }
    }
}

void ExternalTrace::invalidate(const IAG::Subgraph &subgraph) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto callback = _trace->subgraph_destroy) {
            callback(_context, cf_subgraph);
        }
    }
}

void ExternalTrace::destroy(const IAG::Subgraph &subgraph) {}

void ExternalTrace::add_child(const IAG::Subgraph &subgraph, const IAG::Subgraph &child) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto cf_child = child.to_cf()) {
            if (auto callback = _trace->subgraph_add_child) {
                callback(_context, cf_subgraph, cf_child);
            }
        }
    }
}

void ExternalTrace::remove_child(const IAG::Subgraph &subgraph, const IAG::Subgraph &child) {
    if (auto cf_subgraph = subgraph.to_cf()) {
        if (auto cf_child = child.to_cf()) {
            if (auto callback = _trace->subgraph_remove_child) {
                callback(_context, cf_subgraph, cf_child);
            }
        }
    }
}

void ExternalTrace::added(IAG::data::ptr<IAG::Node> node) {
    if (auto callback = _trace->node_added) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::add_edge(IAG::data::ptr<IAG::Node> node, IAG::AttributeID input, IAGInputOptions input_options) {
    if (auto callback = _trace->node_add_edge) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), IAGAttribute(IAG::AttributeID(input)), input_options);
    }
}

void ExternalTrace::remove_edge(IAG::data::ptr<IAG::Node> node, uint32_t input_index) {
    if (auto callback = _trace->node_remove_edge) {
        if (IAG::AttributeID(node).subgraph()) {
            auto input = node->input_edges()[input_index].attribute;
            callback(_context, IAGAttribute(IAG::AttributeID(node)), IAGAttribute(input));
        }
    }
}

void ExternalTrace::set_edge_pending(IAG::data::ptr<IAG::Node> node, uint32_t input_index, bool pending) {
    if (auto callback = _trace->node_set_edge_pending) {
        if (IAG::AttributeID(node).subgraph()) {
            auto input = node->input_edges()[input_index].attribute;
            callback(_context, IAGAttribute(IAG::AttributeID(node)), IAGAttribute(input), pending);
        }
    }
}

void ExternalTrace::set_dirty(IAG::data::ptr<IAG::Node> node, bool dirty) {
    if (auto callback = _trace->node_set_dirty) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), dirty);
    }
}

void ExternalTrace::set_pending(IAG::data::ptr<IAG::Node> node, bool pending) {
    if (auto callback = _trace->node_set_pending) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), pending);
    }
}

void ExternalTrace::set_value(IAG::data::ptr<IAG::Node> node, const void *value) {
    if (auto callback = _trace->node_set_value) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), value);
    }
}

void ExternalTrace::mark_value(IAG::data::ptr<IAG::Node> node) {
    if (auto callback = _trace->node_mark_value) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)));
    }
}

void ExternalTrace::added(IAG::data::ptr<IAG::IndirectNode> indirect_node) {
    if (auto callback = _trace->indirect_node_added) {
        callback(_context, IAGAttribute(IAG::AttributeID(indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::set_source(IAG::data::ptr<IAG::IndirectNode> indirect_node, IAG::AttributeID source) {
    if (auto callback = _trace->indirect_node_set_source) {
        callback(_context, IAGAttribute(IAG::AttributeID(indirect_node)),
                 IAGAttribute(source)); // TODO: check sets kind
    }
}

void ExternalTrace::set_dependency(IAG::data::ptr<IAG::IndirectNode> indirect_node, IAG::AttributeID dependency) {
    if (auto callback = _trace->indirect_node_set_dependency) {
        callback(_context, IAGAttribute(IAG::AttributeID(indirect_node)),
                 IAGAttribute(dependency)); // TODO: check sets kind
    }
}

void ExternalTrace::mark_profile(const IAG::Graph &graph, uint32_t event_id) {
    if (auto callback = _trace->profile_mark) {
        const char *event_name = graph.key_name(event_id);
        callback(_context, event_name);
    }
}

void ExternalTrace::custom_event(const IAG::Graph::Context &context, const char *event_name, const void *value,
                                 const IAG::swift::metadata &type) {
    if (_trace->version < IAGTraceTypeVersionCustom) {
        return;
    }
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->custom_event) {
            callback(_context, cf_context, event_name, value, IAGTypeID(&type));
        }
    }
}

void ExternalTrace::named_event(const IAG::Graph::Context &context, IAGNamedTraceEventID event_id,
                                size_t event_arg_count, const uint32_t *event_args, CFDataRef data,
                                IAGNamedTraceEventFlags flags) {
    if (_trace->version < IAGTraceTypeVersionNamed) {
        return;
    }
    if (auto cf_context = context.to_cf()) {
        if (auto callback = _trace->named_event) {
            callback(_context, cf_context, event_id, event_arg_count, event_args, data, flags);
        }
    }
}

bool ExternalTrace::named_event_enabled(IAGNamedTraceEventID event_id) {
    if (_trace->version < IAGTraceTypeVersionNamed) {
        return false;
    }
    if (auto callback = _trace->named_event_enabled) {
        return callback(_context, event_id);
    }
    return _trace->named_event != nullptr;
}

void ExternalTrace::set_deadline(uint64_t deadline) {
    if (_trace->version < IAGTraceTypeVersionDeadline) {
        return;
    }
    if (auto callback = _trace->set_deadline) {
        callback(_context, deadline);
    }
}

void ExternalTrace::passed_deadline() {
    if (_trace->version < IAGTraceTypeVersionDeadline) {
        return;
    }
    if (auto callback = _trace->passed_deadline) {
        callback(_context);
    }
}

void ExternalTrace::compare_failed(IAG::data::ptr<IAG::Node> node, const void *lhs, const void *rhs,
                                   size_t range_offset, size_t range_size, const IAG::swift::metadata *_Nullable type) {
    if (_trace->version < IAGTraceTypeVersionCompareFailed) {
        return;
    }
    IAGComparisonStateStorage storage = {lhs, rhs, range_offset, range_size, IAGTypeID(&type)};
    if (auto callback = _trace->compare_failed) {
        callback(_context, IAGAttribute(IAG::AttributeID(node)), &storage);
    }
}
