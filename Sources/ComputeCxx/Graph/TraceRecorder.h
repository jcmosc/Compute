#pragma once

#include <uuid/uuid.h>

#include <Utilities/HashTable.h>

#include <ComputeCxx/IAGGraphTracing.h>
#include <Utilities/FreeDeleter.h>

#include "ComputeCxx/IAGBase.h"
#include "ComputeCxx/IAGGraph.h"
#include "Protobuf/Encoder.h"
#include "Trace/Trace.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class Graph::TraceRecorder : public Trace, public Encoder::Delegate {
  private:
    Graph &_graph;
    Encoder _encoder;
    IAGGraphTraceFlags _trace_flags;

    vector<std::unique_ptr<const char, util::free_deleter>, 0, uint64_t> _named_event_subsystems;
    
    util::Table<const uuid_t, uint64_t> _image_offset_cache;
    uuid_t _stack_frame_uuid;

    std::unique_ptr<const char, util::free_deleter> _trace_path = nullptr;
    bool _trace_path_created = false;
    
    uint32_t _num_encoded_types = 1; // skip IAGAttributeNullType
    uint32_t _num_encoded_keys = 0;

    struct NamedEventInfo {
        uint32_t event_id;
        bool enabled;
    };
    vector<NamedEventInfo, 0, uint32_t> _named_event_infos;

  public:
    TraceRecorder(Graph &graph, IAGGraphTraceFlags trace_flags, std::span<const char *> subsystems);
    ~TraceRecorder();

    const char *_Nullable trace_path() const { return _trace_path.get(); };
    
    // MARK: Delegate methods

    int flush_encoder(Encoder &encoder) override;
    
    // MARK: Top-level fields

    void encode_event_begin();
    void encode_event_end();
    void encode_subgraph(const Subgraph &subgraph);
    void encode_types();
    void encode_keys();
    void encode_stack();
    void encode_named_event(uint64_t event_id, const char *event_name, const char *event_subsystem);
    void encode_snapshot();

    // MARK: Event fields
    
    enum class EventType : uint64_t {
        Unknown = 0,

        BeginTrace = 1,
        EndTrace = 2,

        BeginSubgraphUpdate = 3,
        EndSubgraphUpdate = 4,
        BeginNodeUpdate = 5,
        EndNodeUpdate = 6,
        BeginValueUpdate = 7,
        EndValueUpdate = 8,
        BeginGraphUpdate = 9,
        EndGraphUpdate = 10,

        BeginGraphInvalidation = 11,
        EndGraphInvalidation = 12,

        BeginModifyNode = 13,
        EndModifyNode = 14,

        BeginEvent = 15,
        EndEvent = 16,

        BeginSnapshot = 17,
        EndSnapshot = 18,

        GraphCreated = 32,
        GraphDestroy = 33,
        GraphNeedsUpdate = 34,

        SubgraphCreated = 35,
        SubgraphInvalidate = 36,
        SubgraphAddChild = 37,
        SubgraphRemoveChild = 38,

        NodeAdded = 39,
        NodeSetDirty = 40,
        NodeSetPending = 41,
        NodeSetValue = 42,
        NodeMarkValue = 43,

        IndirectNodeAdded = 44,
        IndirectNodeSetSource = 45,
        IndirectNodeSetDependency = 46,

        NodeAddEdge = 47,
        NodeRemoveEdge = 48,
        NodeSetEdgePending = 49,

        ProfileMark = 50,
        LogMessage = 51,

        CustomEvent = 52,
        SubgraphDestroy = 53,
        NamedEvent = 54,
        SetDeadline = 55,
        PassedDeadline = 56
    };

    void field_event_type(Encoder &encoder, EventType event_type);
    void field_timestamp(Encoder &encoder);
    void field_payload_1(Encoder &encoder, uint64_t payload);
    void field_payload_2(Encoder &encoder, uint64_t payload);
    void field_payload_3(Encoder &encoder, uint64_t payload);
    void field_backtrace(Encoder &encoder);
    void field_data(Encoder &encoder, const void *data, size_t length);
    void field_named_event_id(Encoder &encoder, uint32_t event_id);

    // MARK: Trace methods

    void graph_destroyed() override;
    void trace_removed() override;

    void begin_trace(const Graph &graph) override;
    void end_trace(const Graph &graph) override;
    void sync_trace() override;

    void log_message_v(const char *format, va_list args) override;

    void begin_update(const Subgraph &subgraph, IAGAttributeFlags subgraph_flags) override;
    void end_update(const Subgraph &subgraph) override;
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) override;
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                    IAGGraphUpdateStatus update_status) override;
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

    void add_edge(data::ptr<Node> node, AttributeID input, IAGInputOptions input_options) override;
    void remove_edge(data::ptr<Node> node, uint32_t input_index) override;
    void set_edge_pending(data::ptr<Node> node, uint32_t input_index, bool pending) override;

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
    void named_event(const Graph::Context &context, IAGNamedTraceEventID event_id, uint32_t event_arg_count,
                     const void **event_args, CFDataRef data, uint32_t arg6) override;
    bool named_event_enabled(IAGNamedTraceEventID event_id) override;

    // compare_failed not overridden
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
