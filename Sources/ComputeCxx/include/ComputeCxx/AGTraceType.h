#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGGraph.h>
#include <ComputeCxx/AGSubgraph.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_ENUM(uint64_t, AGTraceTypeVersion) {
    AGTraceTypeVersionInitial = 0,
    AGTraceTypeVersionCustom = 1,
    AGTraceTypeVersionNamed = 2,
    AGTraceTypeVersionDeadline = 3,
    AGTraceTypeVersionCompareFailed = 4,
};

typedef struct AGTraceType {
    AGTraceTypeVersion version;

    void (*_Nullable begin_trace)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable end_trace)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable begin_subgraph_update)(void *_Nullable context, AGSubgraphRef subgraph, uint32_t options);
    void (*_Nullable end_subgraph_update)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable begin_node_update)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable end_node_update)(void *_Nullable context, bool changed);
    void (*_Nullable begin_value_update)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable end_value_update)(void *_Nullable context, AGAttribute attribute, bool changed);
    void (*_Nullable begin_graph_update)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable end_graph_update)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable begin_graph_invalidation)(void *_Nullable context, AGGraphRef graph, AGAttribute attribute);
    void (*_Nullable end_graph_invalidation)(void *_Nullable context, AGGraphRef graph, AGAttribute attribute);

    void (*_Nullable begin_modify_node)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable end_modify_node)(void *_Nullable context, AGAttribute attribute);

    void (*_Nullable begin_event)(void *_Nullable context, AGAttribute attribute, const char *event_name);
    void (*_Nullable end_event)(void *_Nullable context, AGAttribute attribute, const char *event_name);

    void (*_Nullable graph_created)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable graph_destroy)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable graph_needs_update)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable subgraph_created)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable subgraph_destroy)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable subgraph_add_child)(void *_Nullable context, AGSubgraphRef subgraph, AGSubgraphRef child);
    void (*_Nullable subgraph_remove_child)(void *_Nullable context, AGSubgraphRef subgraph, AGSubgraphRef child);

    void (*_Nullable node_added)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable node_add_edge)(void *_Nullable context, AGAttribute attribute, AGAttribute input, AGInputOptions input_options);
    void (*_Nullable node_remove_edge)(void *_Nullable context, AGAttribute attribute, uint32_t index);
    void (*_Nullable node_set_edge_pending)(void *_Nullable context, AGAttribute attribute, AGAttribute input, bool pending);

    void (*_Nullable node_set_dirty)(void *_Nullable context, AGAttribute attribute, bool dirty);
    void (*_Nullable node_set_pending)(void *_Nullable context, AGAttribute attribute, bool pending);
    void (*_Nullable node_set_value)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable node_mark_value)(void *_Nullable context, AGAttribute attribute);

    void (*_Nullable indirect_node_added)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable indirect_node_set_source)(void *_Nullable context, AGAttribute attribute, AGAttribute source);
    void (*_Nullable indirect_node_set_dependency)(void *_Nullable context, AGAttribute attribute, AGAttribute dependency);

    void (*_Nullable profile_mark)(void *_Nullable context, const char *event_name);

    void (*_Nullable custom_event)(void *_Nullable context, AGGraphRef graph, const char *event_name, const void *value,
                                   AGTypeID type);
    void (*_Nullable named_event)(void *_Nullable context, AGGraphRef graph, uint32_t eventID, uint32_t eventArgCount,
                                  const void *eventArgs, CFDataRef data, uint32_t arg6);
    bool (*_Nullable named_event_enabled)(void *_Nullable context);

    void (*_Nullable set_deadline)(void *_Nullable context);
    void (*_Nullable passed_deadline)(void *_Nullable context);

    void (*_Nullable compare_failed)(void *_Nullable context, AGAttribute attribute, AGComparisonState comparisonState);
} AGTraceType;

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
