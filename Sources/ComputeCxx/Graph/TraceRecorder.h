#pragma once

#include <CoreFoundation/CFBase.h>
#include <execinfo.h>

#include "Encoder/Encoder.h"
#include "Trace/Trace.h"
#include "Utilities/HashTable.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::TraceRecorder : public Encoder::Delegate, public Trace {
  private:
    uint64_t _unique_id;

    Encoder::Delegate *_Nonnull _delegate;
    Graph *_graph;
    Encoder _encoder;
    uint8_t _tracing_flags;

    util::InlineHeap<256> _heap;

    vector<const char *, 0, uint64_t> _named_event_subsystems;

    util::Table<const uuid_t, uint64_t> _image_offset_cache;
    uuid_t _stack_frame_uuid;

    const char *_Nullable _trace_path = nullptr;
    bool _trace_file_exists = false;

    uint32_t _num_encoded_types = 1; // skip AGAttributeNullType
    uint32_t _num_encoded_keys = 0;

    struct NamedEventInfo {
        uint32_t event_id;
        bool enabled;
    };
    vector<NamedEventInfo, 0, uint32_t> _named_event_infos;

  public:
    TraceRecorder(Graph *graph, uint8_t tracing_flags, std::span<const char *> subsystems);
    ~TraceRecorder();

    uint64_t unique_id() { return _unique_id; };

    const char *_Nullable trace_path() const { return _trace_path; };

    void encode_types();
    void encode_keys();
    void encode_stack();
    void encode_snapshot();

    void field_timestamp(Encoder &encoder);
    void field_backtrace(Encoder &encoder, uint64_t field);

    int flush_encoder(Encoder &encoder);

    // MARK: Trace methods

    void begin_trace(const Graph &graph);
    void end_trace(const Graph &graph);
    void sync_trace();

    void log_message_v(const char *format, va_list args);

    void begin_update(const Subgraph &subgraph, uint32_t options);
    void end_update(const Subgraph &subgraph);
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options);
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, Graph::UpdateStatus update_status);
    void begin_update(data::ptr<Node> node);
    void end_update(data::ptr<Node> node, bool changed);
    void begin_update(const Graph::Context &context);
    void end_update(const Graph::Context &context);

    void begin_invalidation(const Graph::Context &context, AttributeID attribute);
    void end_invalidation(const Graph::Context &context, AttributeID attribute);

    void begin_modify(data::ptr<Node> node);
    void end_modify(data::ptr<Node> node);

    void begin_event(data::ptr<Node> node, uint32_t event_id);
    void end_event(data::ptr<Node> node, uint32_t event_id);

    void created(const Graph::Context &context);
    void destroy(const Graph::Context &context);
    void needs_update(const Graph::Context &context);

    void created(const Subgraph &subgraph);
    void invalidate(const Subgraph &subgraph);
    void destroy(const Subgraph &subgraph);

    void add_child(const Subgraph &subgraph, const Subgraph &child);
    void remove_child(const Subgraph &subgraph, const Subgraph &child);

    void added(data::ptr<Node> node);

    void add_edge(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags);
    void remove_edge(data::ptr<Node> node, uint32_t input_index);
    void set_edge_pending(data::ptr<Node> node, uint32_t input_index, bool pending);

    void set_dirty(data::ptr<Node> node, bool dirty);
    void set_pending(data::ptr<Node> node, bool pending);

    void set_value(data::ptr<Node> node, const void *value);
    void mark_value(data::ptr<Node> node);

    void added(data::ptr<IndirectNode> indirect_node);

    void set_source(data::ptr<IndirectNode> indirect_node, AttributeID source);
    void set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency);

    void set_deadline(uint64_t deadline);
    void passed_deadline();

    void mark_profile(const Graph &graph, uint32_t options);

    void custom_event(const Graph::Context &context, const char *event_name, const void *value,
                      const swift::metadata &type);
    void named_event(const Graph::Context &context, uint32_t event_id, uint32_t num_event_args, const uint64_t *event_args,
                     CFDataRef data, uint32_t arg6);
    bool named_event_enabled(uint32_t event_id);

    // compare_failed not overridden
};

} // namespace AG

CF_ASSUME_NONNULL_END
