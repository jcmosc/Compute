#include "ExternalTrace.h"

#include "Comparison/AGComparison-Private.h"
#include "ComputeCxx/AGGraph.h"
#include "Graph/Context.h"
#include "Graph/Graph.h"

void ExternalTrace::graph_destroyed() { delete this; };

void ExternalTrace::trace_removed() { delete this; };

void ExternalTrace::begin_trace(const AG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->beginTrace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::end_trace(const AG::Graph &graph) {
    auto cf_graph = graph.primary_context()->to_cf();
    if (auto callback = _trace->endTrace) {
        callback(_context, cf_graph);
    }
}

void ExternalTrace::begin_update(const AG::Subgraph &subgraph,
                                 uint32_t options) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->beginUpdateSubgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::end_update(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->endUpdateSubgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::begin_update(const AG::Graph::UpdateStack &update_stack,
                                 AG::data::ptr<AG::Node> node,
                                 uint32_t options) {
    if (auto callback = _trace->beginUpdate) {
        callback(_context, AGAttribute(AG::AttributeID(node)));
    }
}

void ExternalTrace::end_update(const AG::Graph::UpdateStack &update_stack,
                               AG::data::ptr<AG::Node> node,
                               AGGraphUpdateStatus update_status) {
    if (auto callback = _trace->endUpdate) {
        callback(_context, update_status == AGGraphUpdateStatusChanged);
    }
}

void ExternalTrace::begin_update(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->beginUpdateAttribute) {
        callback(_context); // TODO: check doesn't pass attribute
    }
}

void ExternalTrace::end_update(AG::data::ptr<AG::Node> node, bool changed) {
    if (auto callback = _trace->endUpdateAttribute) {
        callback(_context); // TODO: check doesn't pass attribute
    }
}

void ExternalTrace::begin_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->beginUpdateGraph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::end_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->endUpdateGraph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::begin_invalidation(const AG::Graph::Context &context,
                                       AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->beginInvalidation) {
        callback(_context, cf_context, AGAttribute(attribute));
    }
}

void ExternalTrace::end_invalidation(const AG::Graph::Context &context,
                                     AG::AttributeID attribute) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->endInvalidation) {
        callback(_context, cf_context, AGAttribute(attribute));
    }
}

void ExternalTrace::begin_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->beginModify) {
        callback(_context);
    }
}

void ExternalTrace::end_modify(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->endModify) {
        callback(_context);
    }
}

void ExternalTrace::begin_event(AG::data::ptr<AG::Node> node,
                                uint32_t event_id) {
    if (auto callback = _trace->beginEvent) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, AGAttribute(AG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::end_event(AG::data::ptr<AG::Node> node, uint32_t event_id) {
    if (auto callback = _trace->endEvent) {
        if (auto subgraph = AG::AttributeID(node).subgraph()) {
            const char *event_name = subgraph->graph()->key_name(event_id);
            callback(_context, AGAttribute(AG::AttributeID(node)), event_name);
        }
    }
}

void ExternalTrace::created(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->createdGraph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::destroy(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->destroyGraph) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::needs_update(const AG::Graph::Context &context) {
    auto cf_context = context.to_cf();
    if (auto callback = _trace->needsUpdate) {
        callback(_context, cf_context);
    }
}

void ExternalTrace::created(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->createdSubgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::invalidate(const AG::Subgraph &subgraph) {
    auto cf_subgraph = subgraph.to_cf();
    if (auto callback = _trace->invalidateSubgraph) {
        callback(_context, cf_subgraph);
    }
}

void ExternalTrace::destroy(const AG::Subgraph &subgraph) {}

void ExternalTrace::add_child(const AG::Subgraph &subgraph,
                              const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _trace->addChildSubgraph) {
        callback(_context, cf_subgraph, cf_child);
    }
}

void ExternalTrace::remove_child(const AG::Subgraph &subgraph,
                                 const AG::Subgraph &child) {
    auto cf_subgraph = subgraph.to_cf();
    auto cf_child = subgraph.to_cf();
    if (auto callback = _trace->removeChildSubgraph) {
        callback(_context, cf_subgraph, cf_child);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->addedAttribute) {
        callback(_context);
    }
}

void ExternalTrace::add_edge(AG::data::ptr<AG::Node> node,
                             AG::AttributeID input, uint8_t input_edge_flags) {
    if (auto callback = _trace->addEdge) {
        callback(_context);
    }
}

void ExternalTrace::remove_edge(AG::data::ptr<AG::Node> node,
                                uint32_t input_index) {
    if (auto callback = _trace->removeEdge) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_context);
        }
    }
}

void ExternalTrace::set_edge_pending(AG::data::ptr<AG::Node> node,
                                     uint32_t input_index, bool pending) {
    if (auto callback = _trace->setEdgePending) {
        if (AG::AttributeID(node).subgraph()) {
            callback(_context);
        }
    }
}

void ExternalTrace::set_dirty(AG::data::ptr<AG::Node> node, bool dirty) {
    if (auto callback = _trace->setDirty) {
        callback(_context);
    }
}

void ExternalTrace::set_pending(AG::data::ptr<AG::Node> node, bool pending) {
    if (auto callback = _trace->setPending) {
        callback(_context);
    }
}

void ExternalTrace::set_value(AG::data::ptr<AG::Node> node, const void *value) {
    if (auto callback = _trace->setValue) {
        callback(_context);
    }
}

void ExternalTrace::mark_value(AG::data::ptr<AG::Node> node) {
    if (auto callback = _trace->markValue) {
        callback(_context);
    }
}

void ExternalTrace::added(AG::data::ptr<AG::IndirectNode> indirect_node) {
    if (auto callback = _trace->addedIndirectAttribute) {
        callback(_context, AGAttribute(AG::AttributeID(
                               indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::set_source(AG::data::ptr<AG::IndirectNode> indirect_node,
                               AG::AttributeID source) {
    if (auto callback = _trace->setSource) {
        callback(_context, AGAttribute(AG::AttributeID(
                               indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::set_dependency(
    AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID dependency) {
    if (auto callback = _trace->setDependency) {
        callback(_context, AGAttribute(AG::AttributeID(
                               indirect_node))); // TODO: check sets kind
    }
}

void ExternalTrace::mark_profile(const AG::Graph &graph, uint32_t event_id) {
    if (auto callback = _trace->markProfile) {
        const char *event_name = graph.key_name(event_id);
        callback(_context, event_name);
    }
}

void ExternalTrace::custom_event(const AG::Graph::Context &context,
                                 const char *event_name, const void *value,
                                 const AG::swift::metadata &type) {
    if (_trace->events >= AGTraceEventsCustom) {
        auto cf_context = context.to_cf();
        if (auto callback = _trace->customEvent) {
            callback(_context, cf_context, event_name, value, AGTypeID(&type));
        }
    }
}

void ExternalTrace::named_event(const AG::Graph::Context &context,
                                uint32_t event_id, uint32_t event_arg_count,
                                const void *event_args, CFDataRef data,
                                uint32_t arg6) {
    if (_trace->events >= AGTraceEventsNamed) {
        auto cf_context = context.to_cf();
        if (auto callback = _trace->namedEvent) {
            callback(_context, cf_context, event_id, event_arg_count,
                     event_args, data, arg6);
        }
    }
}

bool ExternalTrace::named_event_enabled(uint32_t event_id) {
    if (_trace->events >= AGTraceEventsNamed) {
        if (auto callback = _trace->namedEventEnabled) {
            return callback(_context);
        }
        return _trace->namedEvent != nullptr;
    }
    return false;
}

void ExternalTrace::set_deadline(uint64_t deadline) {
    if (_trace->events >= AGTraceEventsDeadline) {
        if (auto callback = _trace->setDeadline) {
            callback(_context);
        }
    }
}

void ExternalTrace::passed_deadline() {
    if (_trace->events >= AGTraceEventsDeadline) {
        if (auto callback = _trace->passedDeadline) {
            callback(_context);
        }
    }
}

void ExternalTrace::compare_failed(AG::data::ptr<AG::Node> node,
                                   const void *lhs, const void *rhs,
                                   size_t range_offset, size_t range_size,
                                   const AG::swift::metadata *_Nullable type) {
    if (_trace->events >= AGTraceEventsCompareFailed) {
        AGComparisonStateStorage storage = {lhs, rhs, range_offset, range_size,
                                            AGTypeID(&type)};
        if (auto callback = _trace->compareFailed) {
            callback(_context, AGAttribute(AG::AttributeID(node)), &storage);
        }
    }
}
