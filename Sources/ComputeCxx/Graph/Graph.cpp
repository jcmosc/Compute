#include "Graph.h"

#include <CoreFoundation/CFString.h>

#include "Attribute/AttributeData/Node/Node.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Trace/AGTrace.h"
#include "TraceRecorder.h"
#include "UniqueID/AGUniqueID.h"

namespace AG {

Graph *Graph::_all_graphs = nullptr;
os_unfair_lock Graph::_all_graphs_lock = OS_UNFAIR_LOCK_INIT;

Graph::Graph()
    : _heap(nullptr, 0, 0), _interned_types(nullptr, nullptr, nullptr, nullptr, &_heap),
      _contexts_by_id(nullptr, nullptr, nullptr, nullptr, &_heap), _id(AGMakeUniqueID()) {
    _types.push_back(nullptr);

    static dispatch_once_t make_keys;
    dispatch_once_f(&make_keys, nullptr, [](void *context) { Subgraph::make_current_subgraph_key(); });

    // Prepend this graph
    all_lock();
    _next = _all_graphs;
    _previous = nullptr;
    _all_graphs = this;
    if (_next) {
        _next->_previous = this;
    }
    all_unlock();
}

Graph::~Graph() {
    foreach_trace([this](Trace &trace) {
        trace.end_trace(*this);
        trace.graph_destroyed();
    });

    all_lock();
    if (_previous) {
        _previous->_next = _next;
        if (_next) {
            _next->_previous = _previous;
        }
    } else {
        _all_graphs = _next;
        if (_next) {
            _next->_previous = nullptr;
        }
    }
    all_unlock();

    if (_keys) {
        delete _keys;
    }
}

Graph::Context *Graph::primary_context() const {
    struct Info {
        Context *context;
        uint64_t context_id;
    };
    Info info = {nullptr, UINT64_MAX};
    _contexts_by_id.for_each(
        [](const uint64_t context_id, Context *const context, void *info_ref) {
            auto typed_info_ref = (std::pair<Context *, uint64_t> *)info_ref;
            if (context_id < ((Info *)info_ref)->context_id) {
                ((Info *)info_ref)->context = context;
                ((Info *)info_ref)->context_id = context_id;
            }
        },
        &info);
    return info.context;
}

const AttributeType &Graph::attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const {
    auto &type = attribute_type(attribute->type_id());
    if (ref_out) {
        *ref_out = attribute->get_self(type);
    }
    return type;
}

uint32_t Graph::intern_type(const swift::metadata *metadata, ClosureFunctionVP<const AGAttributeType *> make_type) {
    uint32_t type_id = uint32_t(reinterpret_cast<uintptr_t>(_interned_types.lookup(metadata, nullptr)));
    if (type_id) {
        return type_id;
    }

    AttributeType *type = (AttributeType *)make_type();
    type->init_body_offset();

    static bool prefetch_layouts = []() -> bool {
        char *result = getenv("AG_PREFETCH_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    if (prefetch_layouts) {
        type->prefetch_layout();
    }

    type_id = _types.size();
    if (type_id >= 0xffffff) {
        precondition_failure("overflowed max type id: %u", type_id);
    }
    _types.push_back(std::unique_ptr<AttributeType, AttributeType::deleter>(type));
    _interned_types.insert(metadata, reinterpret_cast<void *>(uintptr_t(type_id)));

    size_t self_size = type->body_metadata().vw_size();
    if (self_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute self: %u bytes, %s", uint(self_size),
                    type->body_metadata().name(false));
    }

    size_t value_size = type->value_metadata().vw_size();
    if (value_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute value: %u bytes, %s -> %s", uint(value_size),
                    type->body_metadata().name(false), type->value_metadata().name(false));
    }

    return type_id;
}

void Graph::did_allocate_node_value(size_t size) {
    // TODO: Not implemented
}

void Graph::did_destroy_node_value(size_t size) {
    // TODO: Not implemented
}

void Graph::update_attribute(AttributeID attribute, bool option) {
    // TODO: Not implemented
}

#pragma mark - Trace

void Graph::start_tracing(AGTraceFlags trace_flags, std::span<const char *> subsystems) {
    if (trace_flags & AGTraceFlagsEnabled && _trace_recorder == nullptr) {
        _trace_recorder = new TraceRecorder(this, trace_flags, subsystems);
        if (trace_flags & AGTraceFlagsPrepare) {
            prepare_trace(*_trace_recorder);
        }
        add_trace(_trace_recorder);

        static dispatch_once_t cleanup;
        dispatch_once(&cleanup, ^{
                          // TODO
                      });
    }
}

void Graph::stop_tracing() {
    if (_trace_recorder) {
        remove_trace(_trace_recorder->id());
        _trace_recorder = nullptr;
    }
}

void Graph::sync_tracing() {
    foreach_trace([](Trace &trace) { trace.sync_trace(); });
}

CFStringRef Graph::copy_trace_path() {
    if (_trace_recorder && _trace_recorder->trace_path()) {
        return CFStringCreateWithCString(0, _trace_recorder->trace_path(), kCFStringEncodingUTF8);
    } else {
        return nullptr;
    }
}

void Graph::prepare_trace(Trace &trace) {
    // TODO: Not implemented
}

void Graph::add_trace(Trace *_Nullable trace) {
    if (trace == nullptr) {
        return;
    }
    trace->begin_trace(*this);
    _traces.push_back(trace);
}

void Graph::remove_trace(uint64_t trace_id) {
    auto iter = std::remove_if(_traces.begin(), _traces.end(),
                               [&trace_id](auto trace) -> bool { return trace->id() == trace_id; });
    if (iter) {
        Trace *trace = *iter;
        trace->end_trace(*this);
        trace->trace_removed();
        _traces.erase(iter);
    }
}

void Graph::all_start_tracing(AGTraceFlags trace_flags, std::span<const char *> span) {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->start_tracing(trace_flags, span);
    }
    all_unlock();
}

void Graph::all_stop_tracing() {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->stop_tracing();
    }
    all_unlock();
}

void Graph::all_sync_tracing() {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->sync_tracing();
    }
    all_unlock();
}

CFStringRef Graph::all_copy_trace_path() {
    CFStringRef result = nullptr;
    all_lock();
    if (_all_graphs) {
        result = _all_graphs->copy_trace_path();
    }
    all_unlock();
    return result;
}

void Graph::trace_assertion_failure(bool all_stop_tracing, const char *format, ...) {
    char *message = nullptr;

    va_list args;
    va_start(args, format);

    bool locked = all_try_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->foreach_trace([&format, &args](Trace &trace) { trace.log_message_v(format, args); });
        if (all_stop_tracing) {
            graph->stop_tracing();
        }
    }
    if (locked) {
        all_unlock();
    }

    va_end(args);
}

#pragma mark - Keys

uint32_t Graph::intern_key(const char *key) {
    if (_keys == nullptr) {
        _keys = new Graph::KeyTable(&_heap);
    }
    const char *found = nullptr;
    uint32_t key_id = _keys->lookup(key, &found);
    if (found == nullptr) {
        key_id = _keys->insert(key);
    }
    return key_id;
}

const char *Graph::key_name(uint32_t key_id) const {
    if (_keys != nullptr && key_id < _keys->size()) {
        return _keys->get(key_id);
    }
    AG::precondition_failure("invalid string key id: %u", key_id);
}

} // namespace AG
