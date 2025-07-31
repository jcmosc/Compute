#include "ExternalTrace.h"

#include "Comparison/AGComparison-Private.h"
#include "ComputeCxx/AGGraph.h"
#include "Graph/Context.h"
#include "Graph/Graph.h"

void ExternalTrace::graph_destroyed() { delete this; };

void ExternalTrace::trace_removed() { delete this; };

void ExternalTrace::begin_trace(const AG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->begin_trace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::end_trace(const AG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->end_trace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::begin_update(const AG::Subgraph &subgraph, uint32_t options) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->begin_update_subgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::end_update(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->end_update_subgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::begin_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                                 uint32_t options) {
    if (auto callback = _trace->begin_update) {
        callback(_context, AGAttribute(AG::AttributeID(node)));
    }
}

void ExternalTrace::end_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                               AGGraphUpdateStatus update_status) {
    if (auto callback = _trace->end_update) {
        callback(_context, update_status == AGGraphUpdateStatusChanged);
    }
}

void ExternalTrace::begin_update(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->begin_update_attribute) {
        callback(_context); // TODO: check doesn't pass attribute
    }
}

void ExternalTrace::end_update(AG::data::ptr<AG::Node> node, bool changed) {
    if (auto callback = _trace->end_update_attribute) {
        callback(_context); // TODO: check doesn't pass attribute
    }
}

void ExternalTrace::begin_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->begin_update_graph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::end_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->end_update_graph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::begin_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->begin_invalidation) {
        callback(_context, cf_context, AGAttribute(attribute));
    }
}

void ExternalTrace::end_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->end_invalidation) {
        callback(_context, cf_context, AGAttribute(attribute));
    }
}

void ExternalTrace::begin_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->begin_modify) {
        callback(_context);
    }
}

void ExternalTrace::end_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->end_modify) {
        callback(_context);
    }
}

void ExternalTrace::begin_event(AG::data::ptr<AG::Node> node, uint32_t event_id) {
    if (auto callback = _trace->begin_event) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, AGAttribute(AG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::end_event(AG::data::ptr<AG::Node> node, uint32_t event_id) {
    if (auto callback = _trace->end_event) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, AGAttribute(AG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::created(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->created_graph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::destroy(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->destroy_graph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::needs_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->needs_update) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::created(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->created_subgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::invalidate(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->invalidate_subgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::destroy(const AG::Subgraph &subgraph) {}

void ExternalTrace::add_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _trace->add_child_subgraph) {
        callback(_context, cf_subgraph, cf_child);
    }
}

void ExternalTrace::remove_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _trace->remove_child_subgraph) {
        callback(_context, cf_subgraph, cf_child);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->added_attribute) {
        callback(_context);
    }
}

void ExternalTrace::add_edge(AG::data::ptr<AG::Node> node, AG::AttributeID input, uint8_t input_edge_flags) {
    if (auto callback = _trace->add_edge) {
        callback(_context);
    }
}

void ExternalTrace::remove_edge(AG::data::ptr<AG::Node> node, uint32_t input_index) {
    if (auto callback = _trace->remove_edge) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_context);
        }
    }
}

void ExternalTrace::set_edge_pending(AG::data::ptr<AG::Node> node, AG::AttributeID input, bool pending) {
    if (auto callback = _trace->set_edge_pending) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_context);
        }
    }
}

void ExternalTrace::set_dirty(AG::data::ptr<AG::Node> node, bool dirty) {
    if (auto callback = _trace->set_dirty) {
        callback(_context);
    }
}

void ExternalTrace::set_pending(AG::data::ptr<AG::Node> node, bool pending) {
    if (auto callback = _trace->set_pending) {
        callback(_context);
    }
}

void ExternalTrace::set_value(AG::data::ptr<AG::Node> node, const void *value) {
    if (auto callback = _trace->set_value) {
        callback(_context);
    }
}

void ExternalTrace::mark_value(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->mark_value) {
        callback(_context);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::IndirectNode> indirect_node) {
    if (auto callback = _trace->added_indirect_attribute) {
        callback(_context, AGAttribute(AG::AttributeID(indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::set_source(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID source) {
    if (auto callback = _trace->set_source) {
        callback(_context, AGAttribute(AG::AttributeID(indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::set_dependency(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID dependency) {
    if (auto callback = _trace->set_dependency) {
        callback(_context, AGAttribute(AG::AttributeID(indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::mark_profile(const AG::Graph &graph, uint32_t event_id) {
    if (auto callback = _trace->mark_profile) {
        const char *event_name = graph.key_name(event_id);
        callback(_context, event_name);
    }
}

void ExternalTrace::custom_event(const AG::Graph::Context &context, const char *event_name, const void *value,
                                 const AG::swift::metadata &type) {
    if (_trace->events >= AGTraceEventsCustom) {
        auto cf_context = context.to_cf();
        if (auto callback = _trace->custom_event) {
            callback(_context, cf_context, event_name, value, AGTypeID(&type));
        }
    }
}

void ExternalTrace::named_event(const AG::Graph::Context &context, uint32_t event_id, uint32_t event_arg_count,
                                const void *event_args, CFDataRef data, uint32_t arg6) {
    if (_trace->events >= AGTraceEventsNamed) {
        auto cf_context = context.to_cf();
        if (auto callback = _trace->named_event) {
            callback(_context, cf_context, event_id, event_arg_count, event_args, data, arg6);
        }
    }
}

bool ExternalTrace::named_event_enabled(uint32_t event_id) {
    if (_trace->events >= AGTraceEventsNamed) {
        if (auto callback = _trace->named_event_enabled) {
            return callback(_context);
        }
        return _trace->named_event != nullptr;
    }
    return false;
}

void ExternalTrace::set_deadline(uint64_t deadline) {
    if (_trace->events >= AGTraceEventsDeadline) {
        if (auto callback = _trace->set_deadline) {
            callback(_context);
        }
    }
}

void ExternalTrace::passed_deadline() {
    if (_trace->events >= AGTraceEventsDeadline) {
        if (auto callback = _trace->passed_deadline) {
            callback(_context);
        }
    }
}

void ExternalTrace::compare_failed(AG::data::ptr<AG::Node> node, const void *lhs, const void *rhs, size_t range_offset,
                                   size_t range_size, const AG::swift::metadata *_Nullable type) {
    if (_trace->events >= AGTraceEventsCompareFailed) {
        AGComparisonStateStorage storage = {lhs, rhs, range_offset, range_size, AGTypeID(&type)};
        if (auto callback = _trace->compare_failed) {
            callback(_context, AGAttribute(AG::AttributeID(node)), &storage);
        }
    }
}
