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

    virtual int flush_encoder(Encoder &encoder) override;

    // MARK: Trace methods

    void begin_trace(const Graph &graph) override;
    void end_trace(const Graph &graph) override;
    void sync_trace() override;

    void log_message_v(const char *format, va_list args) override;

    void begin_update(const Subgraph &subgraph, uint32_t options) override;
    void end_update(const Subgraph &subgraph) override;
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) override;
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                    Graph::UpdateStatus update_status) override;
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
    void named_event(const Graph::Context &context, uint32_t event_id, uint32_t num_event_args,
                     const uint64_t *event_args, CFDataRef data, uint32_t arg6) override;
    bool named_event_enabled(uint32_t event_id) override;

    // compare_failed not overridden
};

} // namespace AG

CF_ASSUME_NONNULL_END
