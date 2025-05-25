#include "AGGraph-Private.h"

#include <CoreFoundation/CFString.h>
#include <os/lock.h>

#include "Context.h"
#include "Graph.h"
#include "Private/CFRuntime.h"
#include "Trace/ExternalTrace.h"
#include "Utilities/FreeDeleter.h"

namespace {

CFRuntimeClass &graph_type_id() {
    static auto finalize = [](CFTypeRef graph_ref) {
        AGGraphStorage *storage = (AGGraphRef)graph_ref;
        if (!storage->context.invalidated()) {
            storage->context.AG::Graph::Context::~Context();
        }
    };
    static CFRuntimeClass klass = {
        0,                // version
        "AGGraphStorage", // className
        NULL,             // init
        NULL,             // copy
        finalize,
        NULL, // equal
        NULL, // hash
        NULL, // copyFormattingDesc
        NULL, // copyDebugDesc,
        NULL, // reclaim
        NULL, // refcount
        0     // requiredAlignment
    };
    return klass;
}

} // namespace

CFTypeID AGGraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&graph_type_id());
    return type;
}

AGGraphRef AGGraphCreate() { return AGGraphCreateShared(nullptr); };

AGGraphRef AGGraphCreateShared(AGGraphRef original) {
    CFIndex extra_bytes = sizeof(struct AGGraphStorage) - sizeof(CFRuntimeBase);
    AGGraphRef instance =
        (AGGraphRef)_CFRuntimeCreateInstance(kCFAllocatorDefault, AGGraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        AG::precondition_failure("memory allocation failure.");
    }

    AG::Graph *graph;
    if (original) {
        if (original->context.invalidated()) {
            AG::precondition_failure("invalidated graph");
        }
        graph = &original->context.graph();
        AG::Graph::retain(graph);
    } else {
        graph = new AG::Graph();
    }

    new (&instance->context) AG::Graph::Context(graph); // performs a retain
    AG::Graph::release(graph);

    instance->context.set_invalidated(false);

    return instance;
};

AGUnownedGraphRef AGGraphGetGraphContext(AGGraphRef graph) {
    AG::Graph::Context *graph_context = AG::Graph::Context::from_cf(graph);
    AG::Graph *unowned_graph = &graph_context->graph();
    return reinterpret_cast<AGUnownedGraphRef>(unowned_graph);
}

AGGraphRef AGGraphContextGetGraph(AGUnownedGraphContextRef storage) {
    auto graph_context = reinterpret_cast<AG::Graph::Context *>(storage);
    return graph_context->to_cf();
}

#pragma mark - User context

const void *AGGraphGetContext(AGGraphRef graph) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    return graph_context->context();
}

void AGGraphSetContext(AGGraphRef graph, const void *context) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->set_context(context);
}

#pragma mark - Counter

uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    switch (query) {
        //    case AGGraphCounterQueryNodeCount:
        //        return graph_context->graph().node_count();
        //    case AGGraphCounterQueryTransactionCount:
        //        return graph_context->graph().transaction_count();
        //    case AGGraphCounterQueryUpdateCount:
        //        return graph_context->graph().update_count();
        //    case AGGraphCounterQueryChangeCount:
        //        return graph_context->graph().change_count();
    case AGGraphCounterQueryContextID:
        return graph_context->id();
    case AGGraphCounterQueryGraphID:
        return graph_context->graph().id();
        //    case AGGraphCounterQueryContextThreadUpdating:
        //        return graph_context->is_thread_updating();
        //    case AGGraphCounterQueryThreadUpdating:
        //        return graph_context->graph().is_thread_updating();
        //    case AGGraphCounterQueryContextNeedsUpdate:
        //        return graph_context->needs_update();
        //    case AGGraphCounterQueryNeedsUpdate:
        //        return graph_context->graph().needs_update();
        //    case AGGraphCounterQueryMainThreadUpdateCount:
        //        return graph_context->graph().main_thread_update_count();
        //    case AGGraphCounterQueryNodeTotalCount:
        //        return graph_context->graph().total_node_count();
        //    case AGGraphCounterQuerySubgraphCount:
        //        return graph_context->graph().subgraph_count();
        //    case AGGraphCounterQuerySubgraphTotalCount:
        //        return graph_context->graph().total_subgraph_count();
    default:
        return 0;
    }
}

#pragma mark - Attribute types

uint32_t AGGraphInternAttributeType(AGUnownedGraphRef unowned_graph, AGTypeID type,
                                    const AGAttributeType *(*make_attribute_type)(const void *context AG_SWIFT_CONTEXT)
                                        AG_SWIFT_CC(swift),
                                    const void *make_attribute_type_context) {
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    AG::Graph *graph = reinterpret_cast<AG::Graph *>(unowned_graph);
    return graph->intern_type(
        metadata, AG::ClosureFunctionVP<const AGAttributeType *>(make_attribute_type, make_attribute_type_context));
}

#pragma mark - Trace

void AGGraphStartTracing(AGGraphRef graph, AGTraceFlags trace_flags) { AGGraphStartTracing2(graph, trace_flags, NULL); }

void AGGraphStartTracing2(AGGraphRef graph, AGTraceFlags trace_flags, CFArrayRef subsystems) {
    auto subsystems_vector = AG::vector<std::unique_ptr<const char, util::free_deleter>, 0, uint64_t>();
    if (subsystems) {
        auto subsystems_count = CFArrayGetCount(subsystems);
        for (CFIndex index = 0; index < subsystems_count; ++index) {
            CFTypeRef value = CFArrayGetValueAtIndex(subsystems, index);
            if (CFGetTypeID(value) != CFStringGetTypeID()) {
                continue;
            }

            char *subsystem;
            if (CFStringGetCString((CFStringRef)value, subsystem, CFStringGetLength((CFStringRef)value),
                                   kCFStringEncodingUTF8)) {
                subsystems_vector.push_back(std::unique_ptr<const char, util::free_deleter>(subsystem));
            }
        }
    }

    std::span<const char *> subsystems_span =
        std::span<const char *>((const char **)subsystems_vector.data(), subsystems_vector.size());

    if (graph == nullptr) {
        AG::Graph::all_start_tracing(trace_flags, subsystems_span);
        return;
    }

    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().start_tracing(trace_flags, subsystems_span);
}

void AGGraphStopTracing(AGGraphRef graph) {
    if (graph == nullptr) {
        AG::Graph::all_stop_tracing();
        return;
    }

    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().stop_tracing();
}

void AGGraphSyncTracing(AGGraphRef graph) {
    if (graph == nullptr) {
        AG::Graph::all_sync_tracing();
        return;
    }

    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().sync_tracing();
}

CFStringRef AGGraphCopyTracePath(AGGraphRef graph) {
    if (graph == nullptr) {
        return AG::Graph::all_copy_trace_path();
    }

    auto graph_context = AG::Graph::Context::from_cf(graph);
    return graph_context->graph().copy_trace_path();
}

void AGGraphSetTrace(AGGraphRef graph, const AGTraceRef trace, void *context) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().remove_trace(0);

    auto external_trace = new ExternalTrace(0, trace, context);
    graph_context->graph().add_trace(external_trace);
}

void AGGraphResetTrace(AGGraphRef graph) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().remove_trace(0);
}

bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    for (auto trace : graph_context->graph().traces()) {
        if (trace->named_event_enabled(event_id)) {
            return true;
        }
    }
    return false;
}

void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, const void *value, AGTypeID type) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().foreach_trace([&graph_context, &event_name, &value, &type](AG::Trace &trace) {
        trace.custom_event(*graph_context, event_name, value, *reinterpret_cast<const AG::swift::metadata *>(type));
    });
}

void AGGraphAddNamedTraceEvent(AGGraphRef graph, uint32_t event_id, uint32_t event_arg_count, const void *event_args,
                               CFDataRef data, uint32_t arg6) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->graph().foreach_trace(
        [&graph_context, &event_id, &event_arg_count, &event_args, &data, &arg6](AG::Trace &trace) {
            trace.named_event(*graph_context, event_id, event_arg_count, event_args, data, arg6);
        });
}

namespace NamedEvents {

static os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
static AG::vector<std::pair<const char *, const char *>, 0, uint32_t> *names;

} // namespace NamedEvents

const char *AGGraphGetTraceEventName(uint32_t event_id) {
    const char *event_name = nullptr;

    os_unfair_lock_lock(&NamedEvents::lock);
    if (NamedEvents::names != nullptr && event_id < NamedEvents::names->size()) {
        event_name = (*NamedEvents::names)[event_id].second;
    }
    os_unfair_lock_unlock(&NamedEvents::lock);

    return event_name;
}

const char *AGGraphGetTraceEventSubsystem(uint32_t event_id) {
    const char *event_subsystem = nullptr;

    os_unfair_lock_lock(&NamedEvents::lock);
    if (NamedEvents::names != nullptr && event_id < NamedEvents::names->size()) {
        event_subsystem = (*NamedEvents::names)[event_id].first;
    }
    os_unfair_lock_unlock(&NamedEvents::lock);

    return event_subsystem;
}

uint32_t AGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem) {
    os_unfair_lock_lock(&NamedEvents::lock);

    if (!NamedEvents::names) {
        NamedEvents::names = new AG::vector<std::pair<const char *, const char *>, 0, uint32_t>();
        NamedEvents::names->push_back({0, 0}); // Disallow 0 as event ID
    }

    uint32_t event_id = NamedEvents::names->size();
    if (event_subsystem != nullptr) {
        event_subsystem = strdup(event_subsystem);
    }
    event_name = strdup(event_name);
    NamedEvents::names->push_back({event_subsystem, event_name});

    os_unfair_lock_unlock(&NamedEvents::lock);

    return event_id;
}
