#include "TraceRecorder.h"

#include <dlfcn.h>
#include <ptrauth.h>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "ComputeCxx/AGGraph.h"
#include "ComputeCxx/AGTrace.h"
#include "ComputeCxx/AGUniqueID.h"
#include "Context.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Time/Time.h"

namespace AG {

namespace {

uint64_t uuid_hash(const uuid_t key) { return *key; }

bool uuid_equal(const uuid_t a, const uuid_t b) { return uuid_compare(a, b) == 0; }

} // namespace

Graph::TraceRecorder::TraceRecorder(Graph *graph, AGTraceFlags trace_flags, std::span<const char *> subsystems) {
    // TODO: not implemented
}

Graph::TraceRecorder::~TraceRecorder() {
    // TODO: not implemented
}

#pragma mark - Trace methods

void Graph::TraceRecorder::graph_destroyed() { delete this; };

void Graph::TraceRecorder::trace_removed() { delete this; };

void Graph::TraceRecorder::begin_trace(const Graph &graph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_trace(const Graph &graph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::sync_trace() {
    // TODO: not implemented
}

void Graph::TraceRecorder::log_message_v(const char *format, va_list args) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_update(const Subgraph &subgraph, uint32_t options) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_update(const Subgraph &subgraph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                        uint32_t options) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                      AGGraphUpdateStatus update_status) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_update(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_update(data::ptr<Node> node, bool changed) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_update(const Graph::Context &context) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_update(const Graph::Context &context) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_invalidation(const Graph::Context &context, AttributeID attribute) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_invalidation(const Graph::Context &context, AttributeID attribute) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_modify(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_modify(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::begin_event(data::ptr<Node> node, uint32_t event_id) {
    // TODO: not implemented
}

void Graph::TraceRecorder::end_event(data::ptr<Node> node, uint32_t event_id) {
    // TODO: not implemented
}

void Graph::TraceRecorder::created(const Graph::Context &context) {
    // TODO: not implemented
}

void Graph::TraceRecorder::destroy(const Graph::Context &context) {
    // TODO: not implemented
}

void Graph::TraceRecorder::needs_update(const Graph::Context &context) {
    // TODO: not implemented
}

void Graph::TraceRecorder::created(const Subgraph &subgraph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::invalidate(const Subgraph &subgraph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::destroy(const Subgraph &subgraph) {
    // TODO: not implemented
}

void Graph::TraceRecorder::add_child(const Subgraph &subgraph, const Subgraph &child) {
    // TODO: not implemented
}

void Graph::TraceRecorder::remove_child(const Subgraph &subgraph, const Subgraph &child) {
    // TODO: not implemented
}

void Graph::TraceRecorder::added(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::add_edge(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags) {
    // TODO: not implemented
}

void Graph::TraceRecorder::remove_edge(data::ptr<Node> node, uint32_t input_index) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_edge_pending(data::ptr<Node> node, AttributeID input, bool pending) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_dirty(data::ptr<Node> node, bool dirty) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_pending(data::ptr<Node> node, bool pending) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_value(data::ptr<Node> node, const void *value) {
    // TODO: not implemented
}

void Graph::TraceRecorder::mark_value(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::added(data::ptr<IndirectNode> indirect_node) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_source(data::ptr<IndirectNode> indirect_node, AttributeID source) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) {
    // TODO: not implemented
}

void Graph::TraceRecorder::set_deadline(uint64_t deadline) {
    // TODO: not implemented
}

void Graph::TraceRecorder::passed_deadline() {
    // TODO: not implemented
}

void Graph::TraceRecorder::mark_profile(const Graph &graph, uint32_t options) {
    // TODO: not implemented
}

void Graph::TraceRecorder::custom_event(const Graph::Context &context, const char *event_name, const void *value,
                                        const swift::metadata &type) {
    // TODO: not implemented
}

void Graph::TraceRecorder::named_event(const Graph::Context &context, uint32_t event_id, uint32_t event_arg_count,
                                       const void *event_args, CFDataRef data, uint32_t arg6) {
    // TODO: not implemented
}

bool Graph::TraceRecorder::named_event_enabled(uint32_t event_id) {
    // TODO: not implemented
    return false;
}

} // namespace AG
