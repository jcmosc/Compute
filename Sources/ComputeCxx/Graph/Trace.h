#pragma once

#include <CoreFoundation/CFData.h>

#include "Graph.h"

namespace AG {

class Node;
class Subgraph;

class Trace {
  private:
    uint64_t _trace_id;

  public:
    uint64_t trace_id() { return _trace_id; }

    virtual ~Trace();

    // Trace
    virtual void begin_trace(const Graph &graph);
    virtual void end_trace(const Graph &graph);
    virtual void sync_trace();

    // Log
    virtual void log_message_v(const char *format, va_list args);
    void log_message(const char *format, ...);

    // Updates
    virtual void begin_update(const Subgraph &subgraph, uint32_t options);
    virtual void end_update(const Subgraph &subgraph);
    virtual void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options);
    virtual void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                            Graph::UpdateStatus update_status);
    virtual void begin_update(data::ptr<Node> node);
    virtual void end_update(data::ptr<Node> node, bool changed);
    virtual void begin_update(const Graph::Context &context);
    virtual void end_update(const Graph::Context &context);

    virtual void begin_invalidation(const Graph::Context &context, AttributeID attribute);
    virtual void end_invalidation(const Graph::Context &context, AttributeID attribute);

    virtual void begin_modify(data::ptr<Node> node);
    virtual void end_modify(data::ptr<Node> node);

    virtual void begin_event(data::ptr<Node> node, uint32_t event);
    virtual void end_event(data::ptr<Node> node, uint32_t event);

    virtual void created(const Graph::Context &context);
    virtual void destroy(const Graph::Context &context);
    virtual void needs_update(const Graph::Context &context);

    virtual void created(const Subgraph &subgraph);
    virtual void invalidate(const Subgraph &subgraph);
    virtual void destroy(const Subgraph &subgraph);

    virtual void add_child(const Subgraph &subgraph, const Subgraph &child);
    virtual void remove_child(const Subgraph &subgraph, const Subgraph &child);

    virtual void added(data::ptr<Node> node);

    virtual void add_edge(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags);
    virtual void remove_edge(data::ptr<Node> node, uint32_t options);
    virtual void set_edge_pending(data::ptr<Node> node, uint32_t input_index, bool pending);

    virtual void set_dirty(data::ptr<Node> node, bool flag);
    virtual void set_pending(data::ptr<Node> node, bool flag);

    virtual void set_value(data::ptr<Node> node, const void *value);
    virtual void mark_value(data::ptr<Node> node);

    virtual void added(data::ptr<IndirectNode> indirect_node);

    virtual void set_source(data::ptr<IndirectNode> indirect_node, AttributeID source);
    virtual void set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency);

    virtual void set_deadline(uint64_t deadline);
    virtual void passed_deadline();

    virtual void mark_profile(const Graph &graph, uint32_t options);

    virtual void custom_event(const Graph::Context &context, const char *event_name, const void *value,
                              const swift::metadata &type);
    virtual bool named_event(const Graph::Context &context, uint32_t arg2, uint64_t arg3, const uint32_t *arg4,
                             CFDataRef arg5, uint32_t arg6);
    virtual bool named_event_enabled(uint32_t options);

    virtual void compare_failed(data::ptr<Node> node, const void *lhs, const void *rhs, size_t lhs_offset,
                                size_t rhs_offset, const swift::metadata &type);
};

} // namespace AG
