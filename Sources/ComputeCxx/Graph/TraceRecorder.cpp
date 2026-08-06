#include "TraceRecorder.h"

#include <algorithm>
#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#if TARGET_OS_MAC && TARGET_OS_OSX
#include <mach/mach_vm.h>
#endif
#include <ptrauth.h>
#include <ranges>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "ComputeCxx/IAGGraph.h"
#include "ComputeCxx/IAGGraphTracing.h"
#include "Context.h"
#include "Graph/KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Time/Time.h"
#include "UpdateStack.h"

namespace IAG {

namespace {

uint64_t uuid_hash(const uuid_t key) {
    uint64_t hash = 0;
    std::memcpy(&hash, key, sizeof(hash));
    return hash;
}

bool uuid_equal(const uuid_t a, const uuid_t b) { return uuid_compare(a, b) == 0; }

} // namespace

Graph::TraceRecorder::TraceRecorder(Graph &graph, IAGGraphTraceFlags trace_flags, std::span<const char *> subsystems)
    : _graph(graph), _encoder(this, 0x10000), _trace_flags(trace_flags),
      _image_offset_cache(
          uuid_hash, uuid_equal, [](const uuid_t key) { free((unsigned char *)key); }, nullptr, nullptr) {
    for (auto subsystem : subsystems) {
        _named_event_subsystems.push_back(std::unique_ptr<const char, util::free_deleter>(strdup(subsystem)));
    }

    #if TARGET_OS_MAC
    void *array[1] = {(void *)&IAGGraphCreate};
    image_offset image_offsets[1];
    backtrace_image_offsets(array, image_offsets, 1);

    uuid_copy(_stack_frame_uuid, image_offsets[0].uuid);
    #endif
}

Graph::TraceRecorder::~TraceRecorder() { _encoder.flush(); }

#pragma mark - Encoder::Delegate

int Graph::TraceRecorder::flush_encoder(Encoder &encoder) {
    int fd = -1;
    if (_trace_path_created) {
        fd = open(_trace_path.get(), O_WRONLY | O_APPEND, 0666);
    } else {
        _trace_path_created = true;

        const char *trace_file = getenv("IAG_TRACE_FILE");
        if (!trace_file) {
            trace_file = "trace";
        }

        const char *dir = getenv("TMPDIR");
        if (!dir || !*dir) {
            dir = "/tmp";
        }

        const char *separator = dir[strlen(dir) - 1] == '/' ? "" : "/";

        char *attempted_trace_path = nullptr;
        for (int attempt = 1; attempt <= 999; ++attempt) {
            asprintf(&attempted_trace_path, "%s%s%s-%04d.iag-trace", dir, separator, trace_file, attempt);
            fd = open(attempted_trace_path, O_WRONLY | O_CREAT | O_EXCL, 0666);
            if (fd != -1) {
                break;
            }
            if (attempted_trace_path) {
                free(attempted_trace_path);
                attempted_trace_path = nullptr;
            }
            if (errno != EEXIST) {
                break;
            }
        }

        _trace_path = std::unique_ptr<const char, util::free_deleter>(attempted_trace_path);

        if (_trace_path) {
            platform_log_info(misc_log(), "created trace file %s", _trace_path.get());
            fprintf(stdout, "created trace file \"%s\" (pid %d)\n", _trace_path.get(), getpid());
        } else {
            fprintf(stdout, "failed to create trace file: %s%s%s-XXXX.ag-trace\n", dir, separator, trace_file);
        }
    }
    if (fd == -1) {
        return -1;
    }

    const char *buffer = encoder.buffer().data();
    size_t remaining = encoder.buffer().size();
    while (remaining > 0) {
        ssize_t written = write(fd, buffer, remaining);
        if (written < 0) {
            if (errno == EINTR) {
                // try again on interrupted error
                continue;
            }
            unlink(_trace_path.get());
            break;
        }
        buffer += written;
        remaining -= written;
    }
    return close(fd);
}

#pragma mark - Top level fields

#define MESSAGE_FIELD_EVENT 1
#define MESSAGE_FIELD_SUBGRAPH 2
#define MESSAGE_FIELD_TYPES 3
#define MESSAGE_FIELD_KEYS 4
#define MESSAGE_FIELD_STACK 5
#define MESSAGE_FIELD_NAMED_EVENT 6

void Graph::TraceRecorder::encode_event_begin() { _encoder.encode_field_begin(MESSAGE_FIELD_EVENT); };

void Graph::TraceRecorder::encode_event_end() { _encoder.encode_field_end(); };

void Graph::TraceRecorder::encode_subgraph(const IAG::Subgraph &subgraph) {
    _encoder.encode_field_begin(MESSAGE_FIELD_SUBGRAPH);
    subgraph.encode(_encoder);
    _encoder.encode_field_end();
}

void Graph::TraceRecorder::encode_types() {
    while (_num_encoded_types < _graph._types.size()) {
        auto attribute_type = _graph.attribute_type(_num_encoded_types);

        _encoder.encode_field_begin(MESSAGE_FIELD_TYPES);
        _encoder.encode_field_varint(1, _num_encoded_types);
        auto body_type_name = attribute_type.body_metadata().name(false);
        _encoder.encode_field_data(2, body_type_name, strlen(body_type_name));
        auto value_type_name = attribute_type.value_metadata().name(false);
        _encoder.encode_field_data(3, value_type_name, strlen(value_type_name));
        _encoder.encode_field_varint(4, attribute_type.body_metadata().vw_size());
        _encoder.encode_field_varint(5, attribute_type.value_metadata().vw_size());
        _encoder.encode_field_varint(6, attribute_type.flags());
        _encoder.encode_field_end();

        _num_encoded_types += 1;
    }
}

void Graph::TraceRecorder::encode_keys() {
    if (_graph._keys == nullptr) {
        return;
    }
    while (_num_encoded_keys < _graph._keys->size()) {
        if (auto key_name = _graph.key_name(_num_encoded_keys)) {
            _encoder.encode_field_begin(MESSAGE_FIELD_KEYS);
            _encoder.encode_field_varint(1, _num_encoded_keys);
            _encoder.encode_field_data(2, key_name, strlen(key_name));
            _encoder.encode_field_end();
        }
        _num_encoded_keys += 1;
    }
}

void Graph::TraceRecorder::encode_stack() {
    auto first_update = current_update();
    if (first_update == 0) {
        return;
    }

    _encoder.encode_field_begin(MESSAGE_FIELD_STACK);

    for (auto update = first_update; update != nullptr; update = update.get()->next()) {
        for (auto &frame : std::ranges::reverse_view(update.get()->frames())) {
            _encoder.encode_field_begin(1);
            _encoder.encode_field_varint(1, frame.attribute.offset());
            _encoder.encode_field_varint(2, frame.pending ? 1 : 0);
            _encoder.encode_field_varint(3, frame.cyclic ? 1 : 0);
            _encoder.encode_field_end();
        }
    }

    _encoder.encode_field_end();
}

void Graph::TraceRecorder::encode_named_event(uint64_t event_id, const char *_Nonnull event_name,
                                              const char *_Nonnull event_subsystem) {
    _encoder.encode_field_begin(MESSAGE_FIELD_NAMED_EVENT);
    _encoder.encode_field_varint(1, event_id);
    auto event_name_length = strlen(event_name);
    if (event_name_length > 0) {
        _encoder.encode_field_data(2, event_name, event_name_length);
    }
    auto event_subsystem_length = strlen(event_subsystem);
    if (event_subsystem_length > 0) {
        _encoder.encode_field_data(3, event_subsystem, event_subsystem_length);
    }
    _encoder.encode_field_end();
}

void Graph::TraceRecorder::encode_snapshot() {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_types();
    encode_keys();

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginSnapshot);
    field_timestamp(_encoder);
    encode_event_end();

    for (auto subgraph : _graph.subgraphs()) {
        if (subgraph->is_valid()) {
            encode_subgraph(*subgraph);
        }
    }

    encode_stack();

    encode_event_begin();
    field_event_type(_encoder, EventType::EndSnapshot);
    field_timestamp(_encoder);
    encode_event_end();
}

#pragma mark - Event fields

#define EVENT_FIELD_EVENT_TYPE 1
#define EVENT_FIELD_TIMESTAMP 2
#define EVENT_FIELD_PAYLOAD_1 3
#define EVENT_FIELD_PAYLOAD_2 4
#define EVENT_FIELD_PAYLOAD_3 5
#define EVENT_FIELD_PAYLOAD_4 6
#define EVENT_FIELD_PAYLOAD_5 7
#define EVENT_FIELD_BACKTRACE 8
#define EVENT_FIELD_DATA 9
#define EVENT_FIELD_NAMED_EVENT_ID 10

void Graph::TraceRecorder::field_event_type(Encoder &encoder, EventType event_type) {
    encoder.encode_field_varint(EVENT_FIELD_EVENT_TYPE, static_cast<uint64_t>(event_type));
}

void Graph::TraceRecorder::field_timestamp(Encoder &encoder) {
    auto timestamp = current_time();
    encoder.encode_field_varint(EVENT_FIELD_TIMESTAMP, static_cast<uint64_t>(timestamp));
}

void Graph::TraceRecorder::field_payload_1(Encoder &encoder, uint64_t payload) {
    encoder.encode_field_varint(EVENT_FIELD_PAYLOAD_1, payload);
}

void Graph::TraceRecorder::field_payload_2(Encoder &encoder, uint64_t payload) {
    encoder.encode_field_varint(EVENT_FIELD_PAYLOAD_2, payload);
}

void Graph::TraceRecorder::field_payload_3(Encoder &encoder, uint64_t payload) {
    encoder.encode_field_varint(EVENT_FIELD_PAYLOAD_3, payload);
}

void Graph::TraceRecorder::field_backtrace(Encoder &encoder) {
    if (!(_trace_flags & IAGGraphTraceFlagsBacktrace)) {
        return;
    }

    static int trace_stack_frames = []() -> int {
        char *result = getenv("IAG_TRACE_STACK_FRAMES");
        if (result) {
            return atoi(result);
        }
        return 8;
    }();
    if (trace_stack_frames == 0) {
        return;
    }

    #if TARGET_OS_MAC && TARGET_OS_OSX
    void *stack_frames_buffer[8];
    int stack_frames_size = backtrace(stack_frames_buffer, std::size(stack_frames_buffer));
    
    image_offset image_offsets[8];
    backtrace_image_offsets(stack_frames_buffer, image_offsets, stack_frames_size);    

    int stack_frames = std::min(trace_stack_frames, stack_frames_size);
    for (int frame_index = 0; frame_index < stack_frames; ++frame_index) {
        image_offset image_offset = image_offsets[frame_index];
        if (image_offset.offset && !uuid_is_null(image_offset.uuid) &&
            uuid_compare(image_offset.uuid, _stack_frame_uuid)) {

            encoder.encode_field_begin(EVENT_FIELD_BACKTRACE);

            uuid_t cached_uuid = {};
            uint64_t image_offset_id = _image_offset_cache.lookup(image_offset.uuid, &cached_uuid);
            if (uuid_is_null(cached_uuid)) {
                uuid_t *key = (uuid_t *)malloc(sizeof(uuid_t));
                uuid_copy(*key, image_offset.uuid);

                image_offset_id = _image_offset_cache.count();
                _image_offset_cache.insert(*key, image_offset_id);

                encoder.encode_field_begin(3);

                // don't encode trailing NULL character
                uuid_string_t uuid_string = {};
                uuid_unparse(*key, uuid_string);
                encoder.encode_field_data(1, uuid_string, sizeof(uuid_string_t) - 1);

                Dl_info dl_info;
                if (dladdr(stack_frames_buffer[frame_index], &dl_info)) {
                    if (dl_info.dli_fname) {
                        encoder.encode_field_data(2, dl_info.dli_fname, strlen(dl_info.dli_fname));
                    }
                    if (dl_info.dli_fbase) {
                        encoder.encode_field_varint(3, reinterpret_cast<uint64_t>(dl_info.dli_fbase));

                        // TODO: what is the correct ptrauth key?
                        mach_vm_address_t address =
                            (mach_vm_address_t)ptrauth_strip(dl_info.dli_fbase, ptrauth_key_process_independent_code);
                        mach_vm_size_t size = 0;
                        vm_region_basic_info_data_64_t info;
                        mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
                        mach_port_t object_name = MACH_PORT_NULL;
                        kern_return_t status =
                            mach_vm_region(mach_task_self(), &address, &size, VM_REGION_BASIC_INFO_64,
                                           (vm_region_info_t)&info, &info_count, &object_name);
                        if (object_name) {
                            mach_port_deallocate(mach_task_self(), object_name);
                        }
                        if (status == KERN_SUCCESS) {
                            encoder.encode_field_varint(4, size);
                        }
                    }
                }

                encoder.encode_field_end();
            }

            encoder.encode_field_varint(1, image_offset_id);
            encoder.encode_field_varint(2, image_offset.offset);
            encoder.encode_field_end();
        }
    }
    #endif
}

void Graph::TraceRecorder::field_data(Encoder &encoder, const void *data, size_t length) {
    encoder.encode_field_data(EVENT_FIELD_DATA, data, length);
}

void Graph::TraceRecorder::field_named_event_id(Encoder &encoder, uint32_t event_id) {
    encoder.encode_field_varint(EVENT_FIELD_NAMED_EVENT_ID, event_id);
}

#pragma mark - Trace methods

void Graph::TraceRecorder::graph_destroyed() { delete this; };

void Graph::TraceRecorder::trace_removed() { delete this; };

void Graph::TraceRecorder::begin_trace(const Graph &graph) {
    encode_event_begin();
    field_event_type(_encoder, EventType::BeginTrace);
    field_timestamp(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_trace(const Graph &graph) {
    encode_event_begin();
    field_event_type(_encoder, EventType::EndTrace);
    field_timestamp(_encoder);
    encode_event_end();

    encode_snapshot();
}

void Graph::TraceRecorder::sync_trace() {
    encode_snapshot();
    _encoder.flush();
}

void Graph::TraceRecorder::log_message_v(const char *format, va_list args) {
    char *message = nullptr;
    int result = vasprintf(&message, format, args);
    if (result < 0) {
        precondition_failure("vasprintf failure (%u)", errno);
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::LogMessage);
    field_timestamp(_encoder);
    field_backtrace(_encoder);
    field_data(_encoder, message, strlen(message));
    encode_event_end();

    encode_stack();

    if (message) {
        free(message);
    }
}

void Graph::TraceRecorder::begin_update(const Subgraph &subgraph, IAGAttributeFlags subgraph_flags) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginSubgraphUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_payload_2(_encoder, subgraph_flags);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_update(const Subgraph &subgraph) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndSubgraphUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                        uint32_t options) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginNodeUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, options);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                      IAGGraphUpdateStatus update_status) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndNodeUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, update_status);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_update(data::ptr<Node> node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginValueUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_update(data::ptr<Node> node, bool changed) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndValueUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, changed ? 1 : 0);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_update(const Graph::Context &context) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginGraphUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_update(const Graph::Context &context) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndGraphUpdate);
    field_timestamp(_encoder);
    field_payload_1(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_invalidation(const Graph::Context &context, AttributeID attribute) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginGraphInvalidation);
    field_timestamp(_encoder);
    field_payload_1(_encoder, attribute);
    field_payload_2(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_invalidation(const Graph::Context &context, AttributeID attribute) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndGraphInvalidation);
    field_timestamp(_encoder);
    field_payload_1(_encoder, attribute);
    field_payload_2(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_modify(data::ptr<Node> node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginModifyNode);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_modify(data::ptr<Node> node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndModifyNode);
    field_timestamp(_encoder);
    if (node.offset()) {
        field_payload_1(_encoder, 1);
    }
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::begin_event(data::ptr<Node> node, uint32_t event_id) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::BeginEvent);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, event_id);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::end_event(data::ptr<Node> node, uint32_t event_id) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::EndEvent);
    field_timestamp(_encoder);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, event_id);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::created(const Graph::Context &context) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::GraphCreated);
    field_payload_1(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::destroy(const Graph::Context &context) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::GraphDestroy);
    field_payload_1(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::needs_update(const Graph::Context &context) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::GraphNeedsUpdate);
    field_payload_1(_encoder, context.id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::created(const Subgraph &subgraph) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SubgraphCreated);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_payload_2(_encoder, subgraph.context_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::invalidate(const Subgraph &subgraph) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SubgraphInvalidate);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();

    encode_subgraph(subgraph);
}

void Graph::TraceRecorder::destroy(const Subgraph &subgraph) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SubgraphDestroy);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::add_child(const Subgraph &subgraph, const Subgraph &child) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SubgraphAddChild);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_payload_2(_encoder, child.subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::remove_child(const Subgraph &subgraph, const Subgraph &child) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SubgraphRemoveChild);
    field_payload_1(_encoder, subgraph.subgraph_id());
    field_payload_2(_encoder, child.subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::added(data::ptr<Node> node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeAdded);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, AttributeID(node).subgraph()->subgraph_id());
    field_payload_3(_encoder, node->type_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::add_edge(data::ptr<Node> node, AttributeID input, IAGInputOptions input_options) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeAddEdge);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, input);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::remove_edge(data::ptr<Node> node, uint32_t input_index) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeRemoveEdge);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, node->input_edges()[input_index].attribute);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_edge_pending(data::ptr<Node> node, uint32_t input_index, bool pending) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeSetEdgePending);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, node->input_edges()[input_index].attribute);
    field_payload_3(_encoder, pending ? 1 : 0);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_dirty(data::ptr<Node> node, bool dirty) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeSetDirty);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, dirty ? 1 : 0);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_pending(data::ptr<Node> node, bool pending) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeSetPending);
    field_payload_1(_encoder, node.offset());
    field_payload_2(_encoder, pending ? 1 : 0);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_value(data::ptr<Node> node, const void *value) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeSetValue);
    field_payload_1(_encoder, node.offset());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::mark_value(data::ptr<Node> node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }
    if (!(_trace_flags & IAGGraphTraceFlagsFull)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NodeMarkValue);
    field_payload_1(_encoder, node.offset());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::added(data::ptr<IndirectNode> indirect_node) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::IndirectNodeAdded);
    field_payload_1(_encoder, indirect_node.offset());
    field_payload_2(_encoder, AttributeID(indirect_node).subgraph()->subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_source(data::ptr<IndirectNode> indirect_node, AttributeID source) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::IndirectNodeSetSource);
    field_payload_1(_encoder, indirect_node.offset());
    // FIXME: This shadows param or is there no param>
    field_payload_2(_encoder, indirect_node->source().identifier()); // TODO: identifier()?
    field_payload_3(_encoder, AttributeID(source).subgraph()->subgraph_id());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::IndirectNodeSetDependency);
    field_payload_1(_encoder, indirect_node.offset());
    // FIXME: This shadows param or is there no param>
    field_payload_2(_encoder, indirect_node->to_mutable().dependency());
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::set_deadline(uint64_t deadline) {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::SetDeadline);
    field_timestamp(_encoder);
    field_payload_1(_encoder, deadline & 0xffffffff);
    field_payload_2(_encoder, deadline >> 32);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::passed_deadline() {
    if (_trace_flags & IAGGraphTraceFlagsCustom) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::PassedDeadline);
    field_timestamp(_encoder);
    field_backtrace(_encoder);
    encode_event_end();

    encode_stack();
}

void Graph::TraceRecorder::mark_profile(const Graph &graph, uint32_t options) {
    encode_event_begin();
    field_event_type(_encoder, EventType::ProfileMark);
    field_timestamp(_encoder);
    field_payload_1(_encoder, options);
    field_backtrace(_encoder);
    encode_event_end();
}

void Graph::TraceRecorder::custom_event(const Graph::Context &context, const char *event_name, const void *value,
                                        const swift::metadata &type) {
    encode_event_begin();
    field_event_type(_encoder, EventType::CustomEvent);
    field_timestamp(_encoder);
    field_backtrace(_encoder);
    field_data(_encoder, event_name, strlen(event_name));
    encode_event_end();
}

void Graph::TraceRecorder::named_event(const Graph::Context &context, IAGNamedTraceEventID event_id,
                                       size_t event_arg_count, const uint32_t *event_args, CFDataRef data,
                                       IAGNamedTraceEventFlags flags) {
    if (!named_event_enabled(event_id)) {
        return;
    }

    encode_event_begin();
    field_event_type(_encoder, EventType::NamedEvent);
    field_named_event_id(_encoder, event_id);
    field_timestamp(_encoder);

    if (flags & IAGNamedTraceEventFlagsRecordBacktrace) {
        field_backtrace(_encoder);
        flags &= ~IAGNamedTraceEventFlagsRecordBacktrace;
    }
    field_payload_1(_encoder, flags);
    for (size_t i = 0; i < std::min(event_arg_count, size_t(4)); ++i) {
        uint32_t event_arg = event_args[i];
        _encoder.encode_field_varint(EVENT_FIELD_PAYLOAD_2 + i, event_arg);
    }
    if (data != nullptr) {
        void *ptr = (void *)CFDataGetBytePtr(data);
        uint64_t length = CFDataGetLength(data);
        field_data(_encoder, ptr, length);
    }
    encode_event_end();
}

bool Graph::TraceRecorder::named_event_enabled(IAGNamedTraceEventID event_id) {
    uint32_t index = 0;
    if (!_named_event_infos.empty()) {
        auto pos = std::lower_bound(
            _named_event_infos.begin(), _named_event_infos.end(), event_id,
            [](const NamedEventInfo &info, uint32_t event_id) -> bool { return info.event_id < event_id; });
        if (pos != _named_event_infos.end() && pos->event_id == event_id) {
            return pos->enabled;
        }
        // TODO: specify difference_type on AG::vector::iterator
        index = (uint32_t)(pos - _named_event_infos.begin());
    }

    const char *event_name = IAGGraphGetTraceEventName(event_id);
    if (event_name == nullptr) {
        precondition_failure("invalid named trace event: %u", event_id);
    }

    const char *event_subsystem = IAGGraphGetTraceEventSubsystem(event_id);

    bool enabled = false;
    if (event_subsystem == nullptr || (_trace_flags & IAGGraphTraceFlagsAll)) {
        enabled = true;
    } else {
        for (auto &stored_subsystem : _named_event_subsystems) {
            if (!strcasecmp(stored_subsystem.get(), event_subsystem)) {
                enabled = true;
                break;
            }
        }
    }
    _named_event_infos.insert(_named_event_infos.begin() + index, {event_id, enabled});

    if (!enabled) {
        return false;
    }

    encode_named_event(event_id, event_name, event_subsystem);

    return true;
}

} // namespace IAG
