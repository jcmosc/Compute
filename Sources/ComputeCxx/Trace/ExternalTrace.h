#pragma once

#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGAttribute.h"
#include "ComputeCxx/AGComparison.h"
#include "ComputeCxx/AGGraph.h"
#include "ComputeCxx/AGType.h"
#include "Graph/Context.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"

AG_ASSUME_NONNULL_BEGIN

class ExternalTrace : public AG::Trace {
  public:
  private:
    const AGTrace *_trace;
    void *_Nullable _context;

  public:
    ExternalTrace(AGTrace *trace, void *context) : _trace(trace), _context(context) {};
    ExternalTrace(uint64_t id, const AGTrace *trace, void *context)
        : AG::Trace(id), _trace(trace), _context(context) {};

    void graph_destroyed() override;
    void trace_removed() override;

    void begin_trace(const AG::Graph &graph) override;
    void end_trace(const AG::Graph &graph) override;

    void begin_update(const AG::Subgraph &subgraph, uint32_t options) override;
    void end_update(const AG::Subgraph &subgraph) override;

    void begin_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                      uint32_t options) override;
    void end_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                    AGGraphUpdateStatus update_status) override;
    void begin_update(AG::data::ptr<AG::Node> node) override;
    void end_update(AG::data::ptr<AG::Node> node, bool changed) override;
    void begin_update(const AG::Graph::Context &context) override;
    void end_update(const AG::Graph::Context &context) override;

    void begin_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) override;
    void end_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute) override;

    void begin_modify(AG::data::ptr<AG::Node> node) override;
    void end_modify(AG::data::ptr<AG::Node> node) override;

    void begin_event(AG::data::ptr<AG::Node> node, uint32_t event_id) override;
    void end_event(AG::data::ptr<AG::Node> node, uint32_t event_id) override;

    void created(const AG::Graph::Context &context) override;
    void destroy(const AG::Graph::Context &context) override;
    void needs_update(const AG::Graph::Context &context) override;

    void created(const AG::Subgraph &subgraph) override;
    void invalidate(const AG::Subgraph &subgraph) override;
    void destroy(const AG::Subgraph &subgraph) override;

    void add_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) override;
    void remove_child(const AG::Subgraph &subgraph, const AG::Subgraph &child) override;

    void added(AG::data::ptr<AG::Node> node) override;

    void add_edge(AG::data::ptr<AG::Node> node, AG::AttributeID input, uint8_t input_edge_flags) override;
    void remove_edge(AG::data::ptr<AG::Node> node, uint32_t input_index) override;
    void set_edge_pending(AG::data::ptr<AG::Node> node, AG::AttributeID input, bool pending) override;

    void set_dirty(AG::data::ptr<AG::Node> node, bool dirty) override;
    void set_pending(AG::data::ptr<AG::Node> node, bool pending) override;
    void set_value(AG::data::ptr<AG::Node> node, const void *value) override;
    void mark_value(AG::data::ptr<AG::Node> node) override;

    void added(AG::data::ptr<AG::IndirectNode> indirect_node) override;

    void set_source(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID source) override;
    void set_dependency(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID dependency) override;

    void mark_profile(const AG::Graph &graph, uint32_t event_id) override;

    void custom_event(const AG::Graph::Context &context, const char *event_name, const void *value,
                      const AG::swift::metadata &type) override;
    void named_event(const AG::Graph::Context &context, uint32_t event_id, uint32_t event_arg_count,
                     const void *event_args, CFDataRef data, uint32_t arg6) override;
    bool named_event_enabled(uint32_t event_id) override;

    void set_deadline(uint64_t deadline) override;
    void passed_deadline() override;

    void compare_failed(AG::data::ptr<AG::Node> node, const void *lhs, const void *rhs, size_t range_offset,
                        size_t range_size, const AG::swift::metadata *_Nullable type) override;
};

AG_ASSUME_NONNULL_END
