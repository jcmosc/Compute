#pragma once

#include "ComputeCxx/IAGAttribute.h"
#include "ComputeCxx/IAGBase.h"
#include "ComputeCxx/IAGComparison.h"
#include "ComputeCxx/IAGGraph.h"
#include "ComputeCxx/IAGTraceType.h"
#include "ComputeCxx/IAGType.h"
#include "Graph/Context.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"

IAG_ASSUME_NONNULL_BEGIN

class ExternalTrace : public IAG::Trace {
  public:
  private:
    const IAGTraceTypeRef _trace;
    void *_Nullable _context;

  public:
    ExternalTrace(IAGTraceTypeRef trace, void *context) : _trace(trace), _context(context) {};
    ExternalTrace(uint64_t id, const IAGTraceTypeRef trace, void *context)
        : IAG::Trace(id), _trace(trace), _context(context) {};

    void graph_destroyed() override;
    void trace_removed() override;

    void begin_trace(const IAG::Graph &graph) override;
    void end_trace(const IAG::Graph &graph) override;

    void begin_update(const IAG::Subgraph &subgraph, uint32_t options) override;
    void end_update(const IAG::Subgraph &subgraph) override;

    void begin_update(const IAG::Graph::UpdateStack &update_stack, IAG::data::ptr<IAG::Node> node,
                      uint32_t options) override;
    void end_update(const IAG::Graph::UpdateStack &update_stack, IAG::data::ptr<IAG::Node> node,
                    IAGGraphUpdateStatus update_status) override;
    void begin_update(IAG::data::ptr<IAG::Node> node) override;
    void end_update(IAG::data::ptr<IAG::Node> node, bool changed) override;
    void begin_update(const IAG::Graph::Context &context) override;
    void end_update(const IAG::Graph::Context &context) override;

    void begin_invalidation(const IAG::Graph::Context &context, IAG::AttributeID attribute) override;
    void end_invalidation(const IAG::Graph::Context &context, IAG::AttributeID attribute) override;

    void begin_modify(IAG::data::ptr<IAG::Node> node) override;
    void end_modify(IAG::data::ptr<IAG::Node> node) override;

    void begin_event(IAG::data::ptr<IAG::Node> node, uint32_t event_id) override;
    void end_event(IAG::data::ptr<IAG::Node> node, uint32_t event_id) override;

    void created(const IAG::Graph::Context &context) override;
    void destroy(const IAG::Graph::Context &context) override;
    void needs_update(const IAG::Graph::Context &context) override;

    void created(const IAG::Subgraph &subgraph) override;
    void invalidate(const IAG::Subgraph &subgraph) override;
    void destroy(const IAG::Subgraph &subgraph) override;

    void add_child(const IAG::Subgraph &subgraph, const IAG::Subgraph &child) override;
    void remove_child(const IAG::Subgraph &subgraph, const IAG::Subgraph &child) override;

    void added(IAG::data::ptr<IAG::Node> node) override;

    void add_edge(IAG::data::ptr<IAG::Node> node, IAG::AttributeID input, IAGInputOptions input_options) override;
    void remove_edge(IAG::data::ptr<IAG::Node> node, uint32_t input_index) override;
    void set_edge_pending(IAG::data::ptr<IAG::Node> node, IAG::AttributeID input, bool pending) override;

    void set_dirty(IAG::data::ptr<IAG::Node> node, bool dirty) override;
    void set_pending(IAG::data::ptr<IAG::Node> node, bool pending) override;
    void set_value(IAG::data::ptr<IAG::Node> node, const void *value) override;
    void mark_value(IAG::data::ptr<IAG::Node> node) override;

    void added(IAG::data::ptr<IAG::IndirectNode> indirect_node) override;

    void set_source(IAG::data::ptr<IAG::IndirectNode> indirect_node, IAG::AttributeID source) override;
    void set_dependency(IAG::data::ptr<IAG::IndirectNode> indirect_node, IAG::AttributeID dependency) override;

    void mark_profile(const IAG::Graph &graph, uint32_t event_id) override;

    void custom_event(const IAG::Graph::Context &context, const char *event_name, const void *value,
                      const IAG::swift::metadata &type) override;
    void named_event(const IAG::Graph::Context &context, uint32_t event_id, uint32_t event_arg_count,
                     const void *event_args, CFDataRef data, uint32_t arg6) override;
    bool named_event_enabled(uint32_t event_id) override;

    void set_deadline(uint64_t deadline) override;
    void passed_deadline() override;

    void compare_failed(IAG::data::ptr<IAG::Node> node, const void *lhs, const void *rhs, size_t range_offset,
                        size_t range_size, const IAG::swift::metadata *_Nullable type) override;
};

IAG_ASSUME_NONNULL_END
