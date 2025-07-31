#pragma once

#include <CoreFoundation/CFBase.h>
#include <execinfo.h>

#include <Utilities/HashTable.h>

#include "ComputeCxx/AGGraph.h"
#include "Trace/Trace.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::TraceRecorder : public Trace {
  public:
    TraceRecorder(Graph *graph, AGTraceFlags trace_flags, std::span<const char *> subsystems);
    ~TraceRecorder();

    uint64_t id() { return _id; };

    const char *_Nullable trace_path() const {
        // TODO: not implemented
        return nullptr;
    };

    // MARK: Trace methods

    void graph_destroyed() override;
    void trace_removed() override;

    void begin_trace(const Graph &graph) override;
    void end_trace(const Graph &graph) override;
    void sync_trace() override;

    void log_message_v(const char *format, va_list args) override;

    void begin_update(const Subgraph &subgraph, uint32_t options) override;
    void end_update(const Subgraph &subgraph) override;
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) override;
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                    AGGraphUpdateStatus update_status) override;
    void begin_update(data::ptr<Node> node) override;
    void end_update(data::ptr<Node> node, bool changed) override;
    void begin_update(const Graph::Context &context) override;
    void end_update(const Graph::Context &context) override;

    void begin_invalidation(const Graph::Context &context, AttributeID attribute) override;
    void end_invalidation(const Graph::Context &context, AttributeID attribute) override;

    void begin_modify(data::ptr<Node> node) override;
    void end_modify(data::ptr<Node> node) override;

    void begin_event(data::ptr<Node> node, uint32_t event_id) override;
    void end_event(data::ptr<Node> node, uint32_t event_id) override;

    void created(const Graph::Context &context) override;
    void destroy(const Graph::Context &context) override;
    void needs_update(const Graph::Context &context) override;

    void created(const Subgraph &subgraph) override;
    void invalidate(const Subgraph &subgraph) override;
    void destroy(const Subgraph &subgraph) override;

    void add_child(const Subgraph &subgraph, const Subgraph &child) override;
    void remove_child(const Subgraph &subgraph, const Subgraph &child) override;

    void added(data::ptr<Node> node) override;

    void add_edge(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags) override;
    void remove_edge(data::ptr<Node> node, uint32_t input_index) override;
    void set_edge_pending(data::ptr<Node> node, AttributeID input, bool pending) override;

    void set_dirty(data::ptr<Node> node, bool dirty) override;
    void set_pending(data::ptr<Node> node, bool pending) override;

    void set_value(data::ptr<Node> node, const void *value) override;
    void mark_value(data::ptr<Node> node) override;

    void added(data::ptr<IndirectNode> indirect_node) override;

    void set_source(data::ptr<IndirectNode> indirect_node, AttributeID source) override;
    void set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) override;

    void set_deadline(uint64_t deadline) override;
    void passed_deadline() override;

    void mark_profile(const Graph &graph, uint32_t options) override;

    void custom_event(const Graph::Context &context, const char *event_name, const void *value,
                      const swift::metadata &type) override;
    void named_event(const Graph::Context &context, uint32_t event_id, uint32_t event_arg_count, const void *event_args,
                     CFDataRef data, uint32_t arg6) override;
    bool named_event_enabled(uint32_t event_id) override;

    // compare_failed not overridden
};

} // namespace AG

CF_ASSUME_NONNULL_END
