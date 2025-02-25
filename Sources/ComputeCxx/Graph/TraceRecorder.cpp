#include "TraceRecorder.h"

#include <dlfcn.h>
#include <mach/mach_vm.h>
#include <os/log.h>
#include <ptrauth.h>

#include "AGGraph.h"
#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Context.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Time/Time.h"
#include "Trace/AGTrace.h"
#include "UniqueID/AGUniqueID.h"
#include "UpdateStack.h"

namespace AG {

namespace {

uint64_t uuid_hash(const uuid_t key) { return *key; }

bool uuid_equal(const uuid_t a, const uuid_t b) { return uuid_compare(a, b) == 0; }

} // namespace

Graph::TraceRecorder::TraceRecorder(Graph *graph, uint8_t options, std::span<const char *> subsystems)
    : _graph(graph), _options(options), _heap(_heap_inline_buffer, 256, 0),
      _image_offset_cache(uuid_hash, uuid_equal, nullptr, nullptr, &_heap), _encoder(_delegate, 0x10000) {
    _unique_id = AGMakeUniqueID();

    _delegate = this;

    for (auto subsystem : subsystems) {
        _named_event_subsystems.push_back(strdup(subsystem));
    }

    void *array[1] = {(void *)&AGGraphCreate};
    image_offset image_offsets[1];
    backtrace_image_offsets(array, image_offsets, 1);

    uuid_copy(_stack_frame_uuid, image_offsets[0].uuid);
}

Graph::TraceRecorder::~TraceRecorder() {
    _encoder.flush();

    if (_trace_path) {
        free((void *)_trace_path);
        _trace_path = nullptr;
    }

    for (auto iter = _named_event_subsystems.begin(), end = _named_event_subsystems.end(); iter != end; ++iter) {
        if (*iter) {
            free((void *)*iter);
        }
        *iter = nullptr;
    }

    void *array[1] = {(void *)&AGGraphCreate};
    image_offset image_offsets[1];
    backtrace_image_offsets(array, image_offsets, 1);

    uuid_copy(_stack_frame_uuid, image_offsets[0].uuid);
}

void Graph::TraceRecorder::encode_types() {
    while (_num_encoded_types < _graph->_types.size()) {
        auto attribute_type = _graph->attribute_type(_num_encoded_types);

        _encoder.encode_varint(0x1a);
        _encoder.begin_length_delimited();

        if (_num_encoded_types) {
            _encoder.encode_varint(8);
            _encoder.encode_varint(_num_encoded_types);
        }
        auto self_metadata_name = attribute_type.self_metadata().name(false);
        auto self_metadata_length = strlen(self_metadata_name);
        if (self_metadata_length) {
            _encoder.encode_varint(0x12);
            _encoder.encode_data((void *)self_metadata_name, self_metadata_length);
        }
        auto value_metadata_name = attribute_type.value_metadata().name(false);
        auto value_metadata_length = strlen(value_metadata_name);
        if (value_metadata_length) {
            _encoder.encode_varint(0x1a);
            _encoder.encode_data((void *)value_metadata_name, value_metadata_length);
        }
        auto self_size = attribute_type.self_metadata().vw_size();
        if (self_size > 0) {
            _encoder.encode_varint(0x20);
            _encoder.encode_varint(self_size);
        }
        auto value_size = attribute_type.value_metadata().vw_size();
        if (value_size > 0) {
            _encoder.encode_varint(0x28);
            _encoder.encode_varint(value_size);
        }
        auto flags = attribute_type.flags();
        if (flags) {
            _encoder.encode_varint(0x30);
            _encoder.encode_varint(flags);
        }

        _encoder.end_length_delimited();

        _num_encoded_types += 1;
    }
}

void Graph::TraceRecorder::encode_keys() {
    if (_graph->_keys == nullptr) {
        return;
    }
    while (_num_encoded_keys < _graph->_keys->size()) {
        if (auto key_name = _graph->key_name(_num_encoded_keys)) {
            _encoder.encode_varint(0x22);
            _encoder.begin_length_delimited();
            if (_num_encoded_keys) {
                _encoder.encode_varint(8);
                _encoder.encode_varint(_num_encoded_keys);
            }
            auto length = strlen(key_name);
            if (length) {
                _encoder.encode_varint(0x12);
                _encoder.encode_data((void *)key_name, length);
            }
            _encoder.end_length_delimited();
        }
        _num_encoded_keys += 1;
    }
}

void Graph::TraceRecorder::encode_stack() {
    auto first_update = current_update();
    if (first_update == 0) {
        return;
    }

    _encoder.encode_varint(0x2a);
    _encoder.begin_length_delimited();

    for (auto update = first_update; update != nullptr; update = update.get()->previous()) {
        auto frames = update.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {
            _encoder.encode_varint(10);
            _encoder.begin_length_delimited();

            if (frame->attribute) {
                _encoder.encode_varint(8);
                _encoder.encode_varint(frame->attribute);
            }
            if (frame->needs_update) {
                _encoder.encode_varint(0x10);
                _encoder.encode_varint(1);
            }
            if (frame->cyclic) {
                _encoder.encode_varint(0x18);
                _encoder.encode_varint(1);
            }

            _encoder.end_length_delimited();
        }
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::encode_snapshot() {
    if (_options & TraceFlags::Custom) {
        return;
    }

    encode_types();
    encode_keys();

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x11);

    field_timestamp(_encoder);

    for (auto subgraph : _graph->subgraphs()) {
        if (subgraph->is_valid()) {
            _encoder.encode_varint(0x12);
            _encoder.begin_length_delimited();
            subgraph->encode(_encoder);
            _encoder.end_length_delimited();
        }
    }

    encode_stack();

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x12);
    field_timestamp(_encoder);
    _encoder.end_length_delimited();

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::field_timestamp(Encoder &encoder) {
    auto time = current_time();
    if (time != 0.0) {
        encoder.encode_varint(0x11);
        encoder.encode_fixed64(time);
    }
}

void Graph::TraceRecorder::field_backtrace(Encoder &encoder, uint64_t field_number) {
    if ((_options & TraceFlags::Backtrace) == 0) {
        return;
    }

    void *stack_frames_buffer[8];
    auto stack_frames_size = backtrace(stack_frames_buffer, sizeof(stack_frames_buffer));

    image_offset image_offsets[8];
    backtrace_image_offsets(stack_frames_buffer, image_offsets, stack_frames_size);

    static uint64_t n_stack_frames = []() -> uint64_t {
        char *result = getenv("AG_TRACE_STACK_FRAMES");
        if (result) {
            return atoi(result);
        }
        return 8;
    }();

    if (n_stack_frames == 0) {
        return;
    }

    for (uint64_t frame_index = 0; frame_index < stack_frames_size; ++frame_index) {
        image_offset image_offset = image_offsets[frame_index];
        if (image_offset.offset && !uuid_is_null(image_offset.uuid) &&
            uuid_compare(image_offset.uuid, _stack_frame_uuid)) {

            _encoder.encode_varint(field_number * 8 + 2);
            _encoder.begin_length_delimited();

            const uuid_t cached_uuid = {};
            uint64_t image_offset_id = _image_offset_cache.lookup(image_offset.uuid, &cached_uuid);
            if (!cached_uuid) {
                uuid_t *key = _heap.alloc<uuid_t>();
                uuid_copy(*key, image_offset.uuid);

                image_offset_id = _image_offset_cache.count();
                _image_offset_cache.insert(*key, image_offset_id);

                _encoder.encode_varint(0x1a);
                _encoder.begin_length_delimited();

                uuid_string_t uuid_string = {};
                uuid_unparse(*key, uuid_string);

                _encoder.encode_varint(0xa);
                _encoder.encode_data(uuid_string, sizeof(uuid_string_t) - 1); // don't encode trailing NULL character

                Dl_info dl_info;
                if (dladdr(stack_frames_buffer[frame_index], &dl_info)) {
                    if (dl_info.dli_fname) {
                        auto length = strlen(dl_info.dli_fname);
                        _encoder.encode_varint(0x12);
                        _encoder.encode_data((void *)dl_info.dli_fname, length);
                    }
                    if (dl_info.dli_fbase) {
                        _encoder.encode_varint(0x18);
                        _encoder.encode_varint((uintptr_t)dl_info.dli_fbase);

                        // TODO: what is the correct ptrauth key?
                        mach_vm_address_t address =
                            (mach_vm_address_t)ptrauth_strip(dl_info.dli_fbase, ptrauth_key_process_independent_code);
                        mach_vm_size_t size = 0;
                        vm_region_info_t info;
                        mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
                        mach_port_t object_name = MACH_PORT_NULL;
                        kern_return_t status = mach_vm_region(mach_task_self(), &address, &size,
                                                              VM_REGION_BASIC_INFO_64, info, &info_count, &object_name);
                        if (object_name) {
                            mach_port_deallocate(mach_task_self(), object_name);
                        }
                        if (status == KERN_SUCCESS) {
                            if (size) {
                                _encoder.encode_varint(0x20);
                                _encoder.encode_varint(size);
                            }
                        }
                    }
                }

                _encoder.end_length_delimited();
            }

            if (image_offset_id) {
                _encoder.encode_varint(8);
                _encoder.encode_varint(image_offset_id);
            }
            if (image_offset.offset) {
                _encoder.encode_varint(0x10);
                _encoder.encode_varint(image_offset.offset);
            }

            _encoder.end_length_delimited();
        }
    }
}

int Graph::TraceRecorder::flush_encoder(Encoder &encoder) {
    int fd = -1;
    if (_trace_file_exists) {
        fd = open(_trace_path, O_WRONLY | O_APPEND, 0666);
    } else {
        _trace_file_exists = true;

        const char *trace_file = getenv("AG_TRACE_FILE");
        if (!trace_file) {
            trace_file = "trace";
        }

        const char *dir = getenv("TMPDIR");
        if (!dir || !*dir) {
            dir = "/tmp";
        }

        const char *separator = dir[strlen(dir) - 1] == '/' ? "" : "/";

        char *attempted_file_name = nullptr;
        for (int attempt = 1; attempt <= 999; ++attempt) {
            asprintf(&attempted_file_name, "%s%s%s-%04d.ag-trace", dir, separator, trace_file, attempt);
            fd = open(attempted_file_name, O_WRONLY | O_CREAT | O_EXCL, 0666);
            if (fd != -1) {
                break;
            }
            if (attempted_file_name) {
                free(attempted_file_name);
                attempted_file_name = nullptr;
            }
            if (errno != EEXIST) {
                break;
            }
        }

        const char *old_file_name = _trace_path;
        _trace_path = attempted_file_name;
        if (old_file_name) {
            free((void *)old_file_name);
        }

        if (_trace_path) {
            os_log(misc_log(), "created trace file %s", _trace_path);
            fprintf(stdout, "created trace file \"%s\" (pid %d)\n", _trace_path, getpid());
        } else {
            fprintf(stdout, "failed to create trace file: %s%s%s-XXXX.ag-trace\n", dir, separator, trace_file);
        }
    }
    if (fd == -1) {
        return;
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
            unlink(_trace_path);
            break;
        }
        buffer += written;
        remaining -= written;
    }
    return close(fd);
}

#pragma mark - Trace methods

void Graph::TraceRecorder::begin_trace(const Graph &graph) {
    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(1);
    field_timestamp(_encoder);
    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_trace(const Graph &graph) {
    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(2);
    field_timestamp(_encoder);
    _encoder.end_length_delimited();

    encode_snapshot();
}

void Graph::TraceRecorder::sync_trace() {
    encode_snapshot();
    _encoder.flush();
}

void Graph::TraceRecorder::log_message_v(const char *format, va_list args) {
    char *message = nullptr;
    vasprintf(&message, format, args);

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x33);
    field_timestamp(_encoder);
    field_backtrace(_encoder, 8);

    size_t length = strlen(message);
    if (length > 0) {
        _encoder.encode_varint(0x4a);
        _encoder.encode_data(message, length);
    }

    _encoder.end_length_delimited();

    encode_stack();

    if (message) {
        free(message);
    }
}

void Graph::TraceRecorder::begin_update(const Subgraph &subgraph, uint32_t options) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(3);
    field_timestamp(_encoder);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }
    if (options) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(options);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_update(const Subgraph &subgraph) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(4);
    field_timestamp(_encoder);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                        uint32_t options) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(5);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (options) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(options);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                      Graph::UpdateStatus update_status) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(6);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (update_status == Graph::UpdateStatus::Changed) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(Graph::UpdateStatus::Changed);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_update(data::ptr<Node> node) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(7);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_update(data::ptr<Node> node, bool changed) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(8);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (changed) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(true);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_update(const Graph::Context &context) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(9);
    field_timestamp(_encoder);

    if (context.unique_id()) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(context.unique_id());
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_update(const Graph::Context &context) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(10);
    field_timestamp(_encoder);

    if (context.unique_id()) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(context.unique_id());
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_invalidation(const Graph::Context &context, AttributeID attribute) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0xb);
    field_timestamp(_encoder);

    if (attribute) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(attribute);
    }
    if (context.unique_id()) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(context.unique_id());
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_invalidation(const Graph::Context &context, AttributeID attribute) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0xc);
    field_timestamp(_encoder);

    if (attribute) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(attribute);
    }
    if (context.unique_id()) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(context.unique_id());
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_modify(data::ptr<Node> node) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0xd);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_modify(data::ptr<Node> node) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0xe);
    field_timestamp(_encoder);

    if (node || _options & TraceFlags::Custom) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(1);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::begin_event(data::ptr<Node> node, uint32_t event) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0xf);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (event) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(event);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::end_event(data::ptr<Node> node, uint32_t event) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x10);
    field_timestamp(_encoder);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (event) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(event);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::created(const Graph::Context &context) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x20);

    if (context.unique_id()) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(context.unique_id());
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::destroy(const Graph::Context &context) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x21);

    if (context.unique_id()) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(context.unique_id());
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::needs_update(const Graph::Context &context) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x22);

    if (context.unique_id()) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(context.unique_id());
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::created(const Subgraph &subgraph) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x23);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }
    if (subgraph.context_id()) {
        _encoder.encode_varint(0x28);
        _encoder.encode_varint(subgraph.context_id());
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::invalidate(const Subgraph &subgraph) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x24);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();

    _encoder.begin_length_delimited();
    _encoder.encode_varint(0x12);
    subgraph.encode(_encoder);
    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::destroy(const Subgraph &subgraph) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x35);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::add_child(const Subgraph &subgraph, const Subgraph &child) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x25);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }
    auto child_zone_id = child.info().zone_id();
    if (child_zone_id) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(child_zone_id);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::remove_child(const Subgraph &subgraph, const Subgraph &child) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x26);

    auto zone_id = subgraph.info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(zone_id);
    }
    auto child_zone_id = child.info().zone_id();
    if (child_zone_id) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(child_zone_id);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::added(data::ptr<Node> node) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x27);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    auto zone_id = AttributeID(node).subgraph()->info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(zone_id);
    }
    auto type_id = node->type_id();
    if (type_id) {
        _encoder.encode_varint(0x28);
        _encoder.encode_varint(type_id);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::add_edge(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2f);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (input) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(input);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::remove_edge(data::ptr<Node> node, uint32_t input_index) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x30);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    auto input_edge = node->inputs()[input_index];
    if (input_edge.value) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(input_edge.value);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_edge_pending(data::ptr<Node> node, uint32_t input_index, bool pending) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x30);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    auto input_edge = node->inputs()[input_index];
    if (input_edge.value) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(input_edge.value);
    }
    if (pending) {
        _encoder.encode_varint(0x28);
        _encoder.encode_varint(1);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_dirty(data::ptr<Node> node, bool dirty) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x28);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (dirty) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(1);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_pending(data::ptr<Node> node, bool pending) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x29);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }
    if (pending) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(1);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_value(data::ptr<Node> node, const void *value) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2a);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::mark_value(data::ptr<Node> node) {
    if (_options & TraceFlags::Custom) {
        return;
    }
    if ((_options & TraceFlags::Full) == 0) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2b);

    if (node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(node);
    }

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::added(data::ptr<IndirectNode> indirect_node) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2c);

    if (indirect_node) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(AttributeID(indirect_node));
    }
    auto zone_id = AttributeID(indirect_node).subgraph()->info().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(zone_id);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_source(data::ptr<IndirectNode> indirect_node, AttributeID source) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2d);

    _encoder.encode_varint(0x18);
    _encoder.encode_varint(AttributeID(indirect_node));
    auto source_attribute = indirect_node->source().attribute();
    if (source_attribute) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(source_attribute);
    }
    auto zone_id = indirect_node->source().zone_id();
    if (zone_id) {
        _encoder.encode_varint(0x28);
        _encoder.encode_varint(zone_id);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x2e);

    _encoder.encode_varint(0x18);
    _encoder.encode_varint(AttributeID(indirect_node));
    auto dependency_attribute = indirect_node->to_mutable().dependency();
    if (dependency_attribute) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(dependency_attribute);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::set_deadline(uint64_t deadline) {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x37);

    field_timestamp(_encoder);

    if (deadline & 0xffffffff) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(deadline & 0xffffffff);
    }
    if (deadline >> 32) {
        _encoder.encode_varint(0x20);
        _encoder.encode_varint(deadline >> 32);
    }

    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::passed_deadline() {
    if (_options & TraceFlags::Custom) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x38);

    field_timestamp(_encoder);
    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();

    encode_stack();
}

void Graph::TraceRecorder::mark_profile(const Graph &graph, uint32_t options) {
    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x32);

    field_timestamp(_encoder);
    if (options) {
        _encoder.encode_varint(0x18);
        _encoder.encode_varint(options);
    }
    field_backtrace(_encoder, 8);

    _encoder.end_length_delimited();
}

void Graph::TraceRecorder::custom_event(const Graph::Context &context, const char *event_name, const void *value,
                                        const swift::metadata &type) {
    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x34);

    field_timestamp(_encoder);
    field_backtrace(_encoder, 8);

    auto length = strlen(event_name);
    if (length > 0) {
        _encoder.encode_varint(0x4a);
        _encoder.encode_data((void *)event_name, length);
    }

    _encoder.end_length_delimited();
}

bool Graph::TraceRecorder::named_event(const Graph::Context &context, uint32_t arg2, uint32_t num_args,
                                       const uint64_t *event_args, CFDataRef data, uint32_t arg6) {
    if (!named_event_enabled(arg2)) {
        return;
    }

    _encoder.encode_varint(10);
    _encoder.begin_length_delimited();
    _encoder.encode_varint(8);
    _encoder.encode_varint(0x36);

    if (arg2) {
        _encoder.encode_varint(0x50);
        _encoder.encode_varint(arg2);
    }

    field_timestamp(_encoder);

    if (arg6 != 0) {
        if (arg6 & 0x80000000) {
            arg6 &= 0x7fffffff;
        }
        if (arg6) {
            _encoder.encode_varint(0x18);
            _encoder.encode_varint(arg6);
        }
    }

    if (num_args > 3) {
        num_args = 4;
    }
    for (auto i = 0; i < num_args; ++i) {
        uint64_t event_arg = event_args[i];
        if (event_arg) {
            _encoder.encode_varint(0x20 + 8 * i);
            _encoder.encode_varint(event_arg);
        }
    }

    if (data != nullptr) {
        void *ptr = (void *)CFDataGetBytePtr(data);
        uint64_t length = CFDataGetLength(data);
        if (length > 0) {
            _encoder.encode_varint(0x4a);
            _encoder.encode_data(ptr, length);
        }
    }

    _encoder.end_length_delimited();
}

bool Graph::TraceRecorder::named_event_enabled(uint32_t event_id) {
    uint32_t index = 0;
    if (!_named_event_infos.empty()) {
        auto pos = std::lower_bound(
            _named_event_infos.begin(), _named_event_infos.end(), event_id,
            [](const NamedEventInfo &info, uint32_t event_id) -> bool { return info.event_id < event_id; });
        if (pos != _named_event_infos.end() && pos->event_id == event_id) {
            return pos->enabled;
        }
        index = (uint32_t)(pos - _named_event_infos.begin()); // TODO: specify difference_type on AG::vector::iterator
    }

    const char *event_name = AGGraphGetTraceEventName(event_id);
    if (event_name == nullptr) {
        precondition_failure("invalid named trace event: %u", event_id);
    }

    const char *event_subsystem = AGGraphGetTraceEventSubsystem(event_id);

    bool enabled = false;
    if (event_subsystem == nullptr || (_options & TraceFlags::All)) {
        enabled = true;
    } else {
        enabled = (_options & TraceFlags::All) != 0;
        for (auto stored_subsystem : _named_event_subsystems) {
            if (!strcasecmp(stored_subsystem, event_subsystem)) {
                enabled = true;
                break;
            }
        }
    }
    _named_event_infos.insert(_named_event_infos.begin() + index, {event_id, enabled});

    if (!enabled) {
        return false;
    }

    _encoder.encode_varint(0x32);
    _encoder.begin_length_delimited();
    if (event_id) {
        _encoder.encode_varint(8);
        _encoder.encode_varint(event_id);
    }

    auto event_name_length = strlen(event_name);
    if (event_name_length > 0) {
        _encoder.encode_varint(0x12);
        _encoder.encode_data((void *)event_name, event_name_length);
    }
    auto event_subsystem_length = strlen(event_subsystem);
    if (event_subsystem_length > 0) {
        _encoder.encode_varint(0x1a);
        _encoder.encode_data((void *)event_subsystem, event_subsystem_length);
    }

    _encoder.end_length_delimited();

    return true;
}

} // namespace AG
