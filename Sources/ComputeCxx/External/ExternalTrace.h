#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AGAttribute.h"
#include "Graph/Context.h"
#include "Layout/AGComparison.h"
#include "Subgraph/Subgraph.h"
#include "Swift/AGType.h"
#include "Trace/Trace.h"

CF_ASSUME_NONNULL_BEGIN

class ExternalTrace : public AG::Trace {
  public:
    struct Interface {
        uint64_t options;

        void (*_Nullable begin_trace)(void *trace, AGGraphStorage *graph);
        void (*_Nullable end_trace)(void *trace, AGGraphStorage *graph);

        void (*_Nullable begin_update_subgraph)(void *trace, AGSubgraphStorage *subgraph);
        void (*_Nullable end_update_subgraph)(void *trace, AGSubgraphStorage *subgraph);
        void (*_Nullable begin_update_stack)(void *trace, AGAttribute attribute);
        void (*_Nullable end_update_stack)(void *trace, bool changed);
        void (*_Nullable begin_update_node)(void *trace);
        void (*_Nullable end_update_node)(void *trace);
        void (*_Nullable begin_update_context)(void *trace, AGGraphStorage *graph);
        void (*_Nullable end_update_context)(void *trace, AGGraphStorage *graph);

        void (*_Nullable begin_invalidation)(void *trace, AGGraphStorage *graph, AGAttribute attribute);
        void (*_Nullable end_invalidation)(void *trace, AGGraphStorage *graph, AGAttribute attribute);

        void (*_Nullable begin_modify)(void *trace);
        void (*_Nullable end_modify)(void *trace);

        void (*_Nullable begin_event)(void *trace, AGAttribute attribute, const char *event_name);
        void (*_Nullable end_event)(void *trace, AGAttribute attribute, const char *event_name);

        void (*_Nullable created_context)(void *trace, AGGraphStorage *graph);
        void (*_Nullable destroy_context)(void *trace, AGGraphStorage *graph);
        void (*_Nullable needs_update_context)(void *trace, AGGraphStorage *graph);

        void (*_Nullable created_subgraph)(void *trace, AGSubgraphStorage *subgraph);
        void (*_Nullable invalidate_subgraph)(void *trace, AGSubgraphStorage *subgraph);
        void (*_Nullable add_child_subgraph)(void *trace, AGSubgraphStorage *subgraph, AGSubgraphStorage *child);
        void (*_Nullable remove_child_subgraph)(void *trace, AGSubgraphStorage *subgraph, AGSubgraphStorage *child);

        void (*_Nullable added_node)(void *trace);
        void (*_Nullable add_edge)(void *trace);
        void (*_Nullable remove_edge)(void *trace);
        void (*_Nullable set_edge_pending)(void *trace);

        void (*_Nullable set_dirty)(void *trace);
        void (*_Nullable set_pending)(void *trace);
        void (*_Nullable set_value)(void *trace);
        void (*_Nullable mark_value)(void *trace);

        void (*_Nullable added_indirect_node)(void *trace, AGAttribute attribute);
        void (*_Nullable set_source)(void *trace, AGAttribute attribute);
        void (*_Nullable set_dependency)(void *trace, AGAttribute attribute);

        void (*_Nullable mark_profile)(void *trace, const char *event_name);

        void (*_Nullable custom_event)(void *trace, AGGraphStorage *graph, const char *event_name, const void *value,
                                       AGTypeID type);
        void (*_Nullable named_event)(void *trace, AGGraphStorage *graph, uint32_t event_id, uint32_t num_event_args,
                                      const uint64_t *event_args, CFDataRef data, uint32_t arg6);
        bool (*_Nullable named_event_enabled)(void *trace);
        void (*_Nullable set_deadline)(void *trace);
        void (*_Nullable passed_deadline)(void *trace);

        void (*_Nullable compare_failed)(void *trace, AGAttribute attribute, AGComparisonState comparison_state);
    };

  private:
    Interface *_interface;
    void *_trace;

  public:
    ExternalTrace(Interface *interface, void *trace) : _interface(interface), _trace(trace){};
    ExternalTrace(uint64_t trace_id, Interface *interface, void *trace) : _interface(interface), _trace(trace){
        _trace_id = trace_id;
    };

    void begin_trace(const AG::Graph &graph);
    void end_trace(const AG::Graph &graph);

    void begin_update(const AG::Subgraph &subgraph, uint32_t options);
    void end_update(const AG::Subgraph &subgraph);

    void begin_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node, uint32_t options);
    void end_update(const AG::Graph::UpdateStack &update_stack, AG::data::ptr<AG::Node> node,
                    AG::Graph::UpdateStatus update_status);
    void begin_update(AG::data::ptr<AG::Node> node);
    void end_update(AG::data::ptr<AG::Node> node, bool changed);
    void begin_update(const AG::Graph::Context &context);
    void end_update(const AG::Graph::Context &context);

    void begin_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute);
    void end_invalidation(const AG::Graph::Context &context, AG::AttributeID attribute);

    void begin_modify(AG::data::ptr<AG::Node> node);
    void end_modify(AG::data::ptr<AG::Node> node);

    void begin_event(AG::data::ptr<AG::Node> node, uint32_t event_id);
    void end_event(AG::data::ptr<AG::Node> node, uint32_t event_id);

    void created(const AG::Graph::Context &context);
    void destroy(const AG::Graph::Context &context);
    void needs_update(const AG::Graph::Context &context);

    void created(const AG::Subgraph &subgraph);
    void invalidate(const AG::Subgraph &subgraph);
    void destroy(const AG::Subgraph &subgraph);

    void add_child(const AG::Subgraph &subgraph, const AG::Subgraph &child);
    void remove_child(const AG::Subgraph &subgraph, const AG::Subgraph &child);

    void added(AG::data::ptr<AG::Node> node);

    void add_edge(AG::data::ptr<AG::Node> node, AG::AttributeID input, uint8_t input_edge_flags);
    void remove_edge(AG::data::ptr<AG::Node> node, uint32_t input_index);
    void set_edge_pending(AG::data::ptr<AG::Node> node, uint32_t input_index, bool pending);

    void set_dirty(AG::data::ptr<AG::Node> node, bool dirty);
    void set_pending(AG::data::ptr<AG::Node> node, bool pending);
    void set_value(AG::data::ptr<AG::Node> node, const void *value);
    void mark_value(AG::data::ptr<AG::Node> node);

    void added(AG::data::ptr<AG::IndirectNode> indirect_node);

    void set_source(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID source);
    void set_dependency(AG::data::ptr<AG::IndirectNode> indirect_node, AG::AttributeID dependency);

    void mark_profile(const AG::Graph &graph, uint32_t event_id);

    void custom_event(const AG::Graph::Context &context, const char *event_name, const void *value,
                      const AG::swift::metadata &type);
    void named_event(const AG::Graph::Context &context, uint32_t event_id, uint32_t num_event_args,
                     const uint64_t *event_args, CFDataRef data, uint32_t arg6); // TODO: what are these args?
    bool named_event_enabled(uint32_t event_id);

    void set_deadline(uint64_t deadline);
    void passed_deadline();

    void compare_failed(AG::data::ptr<AG::Node> node, const void *lhs, const void *rhs, size_t range_offset,
                        size_t range_size, const AG::swift::metadata *_Nullable type);
};

CF_ASSUME_NONNULL_END
