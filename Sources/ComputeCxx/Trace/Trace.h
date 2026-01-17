#pragma once

#include "ComputeCxx/AGBase.h"

#if TARGET_OS_MAC
#include <CoreFoundation/CFData.h>
#else
#include <SwiftCorelibsCoreFoundation/CFData.h>
#endif

#include "ComputeCxx/AGGraph.h"
#include "ComputeCxx/AGUniqueID.h"
#include "Graph/Graph.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class Node;
class Subgraph;

class Trace {
  protected:
    uint64_t _id;

  public:
    uint64_t id() { return _id; }

    Trace() : _id(AGMakeUniqueID()) {};
    Trace(uint64_t id) : _id(id) {};

    virtual void graph_destroyed() {};
    virtual void trace_removed() {};

    // Trace
    virtual void begin_trace(const Graph &graph) {};
    virtual void end_trace(const Graph &graph) {};
    virtual void sync_trace() {};

    // Log
    virtual void log_message_v(const char *format, va_list args) {};
    void log_message(const char *format, ...);

    // Updates
    virtual void begin_update(const Subgraph &subgraph, uint32_t options) {};
    virtual void end_update(const Subgraph &subgraph) {};
    virtual void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) {};
    virtual void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                            AGGraphUpdateStatus update_status) {};
    virtual void begin_update(data::ptr<Node> node) {};
    virtual void end_update(data::ptr<Node> node, bool changed) {};
    virtual void begin_update(const Graph::Context &context) {};
    virtual void end_update(const Graph::Context &context) {};

    virtual void begin_invalidation(const Graph::Context &context, AttributeID attribute) {};
    virtual void end_invalidation(const Graph::Context &context, AttributeID attribute) {};

    virtual void begin_modify(data::ptr<Node> node) {};
    virtual void end_modify(data::ptr<Node> node) {};

    virtual void begin_event(data::ptr<Node> node, uint32_t event_id) {};
    virtual void end_event(data::ptr<Node> node, uint32_t event_id) {};

    virtual void created(const Graph::Context &context) {};
    virtual void destroy(const Graph::Context &context) {};
    virtual void needs_update(const Graph::Context &context) {};

    virtual void created(const Subgraph &subgraph) {};
    virtual void invalidate(const Subgraph &subgraph) {};
    virtual void destroy(const Subgraph &subgraph) {};

    virtual void add_child(const Subgraph &subgraph, const Subgraph &child) {};
    virtual void remove_child(const Subgraph &subgraph, const Subgraph &child) {};

    virtual void added(data::ptr<Node> node) {};

    virtual void add_edge(data::ptr<Node> node, AttributeID input, AGInputOptions input_options) {};
    virtual void remove_edge(data::ptr<Node> node, uint32_t input_index) {};
    virtual void set_edge_pending(data::ptr<Node> node, AttributeID input, bool pending) {};

    virtual void set_dirty(data::ptr<Node> node, bool dirty) {};
    virtual void set_pending(data::ptr<Node> node, bool pending) {};

    virtual void set_value(data::ptr<Node> node, const void *value) {};
    virtual void mark_value(data::ptr<Node> node) {};

    virtual void added(data::ptr<IndirectNode> indirect_node) {};

    virtual void set_source(data::ptr<IndirectNode> indirect_node, AttributeID source) {};
    virtual void set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) {};

    virtual void set_deadline(uint64_t deadline) {};
    virtual void passed_deadline() {};

    virtual void mark_profile(const Graph &graph, uint32_t options) {};

    virtual void custom_event(const Graph::Context &context, const char *event_name, const void *value,
                              const swift::metadata &type) {};
    virtual void named_event(const Graph::Context &context, uint32_t event_id, uint32_t event_arg_count,
                             const void *event_args, CFDataRef data, uint32_t arg6) {};
    virtual bool named_event_enabled(uint32_t event_id) { return false; };

    virtual void compare_failed(data::ptr<Node> node, const void *lhs, const void *rhs, size_t range_offset,
                                size_t range_size, const swift::metadata *_Nullable type) {};
};

} // namespace AG

AG_ASSUME_NONNULL_END
