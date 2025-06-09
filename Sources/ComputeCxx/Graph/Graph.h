#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <memory>
#include <os/lock.h>
#include <span>
#include <stdint.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "Closure/ClosureFunction.h"
#include "Swift/Metadata.h"
#include "Trace/AGTrace.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"
#include "Vector/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Trace;

class Graph {
  public:
    class Context;
    class KeyTable;
    class TraceRecorder;
    class UpdateStack;

    typedef void (*MainHandler)(void *_Nullable context AG_SWIFT_CONTEXT, void (*trampoline_thunk)(const void *),
                                const void *trampoline) AG_SWIFT_CC(swift);

  private:
    static Graph *_Nullable _all_graphs;
    static os_unfair_lock _all_graphs_lock;

    Graph *_Nullable _next;
    Graph *_Nullable _previous;
    util::Heap _heap;

    // Attribute types
    util::UntypedTable _interned_types;
    vector<std::unique_ptr<AttributeType, AttributeType::deleter>, 0, uint32_t> _types;

    // Contexts
    util::Table<uint64_t, Context *> _contexts_by_id;

    // Trace
    vector<Trace *, 0, uint32_t> _traces;

    // Main thread handler
    MainHandler _Nullable _main_handler;
    const void *_Nullable _main_handler_context;

    // Metrics
    uint64_t _num_nodes = 0;
    uint64_t _num_nodes_total = 0;
    uint64_t _num_subgraphs = 0;
    uint64_t _num_subgraphs_total = 0;

    // Trace recorder
    TraceRecorder *_trace_recorder;

    // Subgraphs
    vector<Subgraph *, 0, uint32_t> _subgraphs;
    vector<Subgraph *, 2, uint32_t> _invalidating_subgraphs;
    bool _deferring_subgraph_invalidation;

    // Keys
    KeyTable *_Nullable _keys;

    // Threads
    uint32_t _ref_count = 1;

    uint64_t _id;

    static void all_lock() { os_unfair_lock_lock(&_all_graphs_lock); };
    static bool all_try_lock() { return os_unfair_lock_trylock(&_all_graphs_lock); };
    static void all_unlock() { os_unfair_lock_unlock(&_all_graphs_lock); };

  public:
    Graph();
    ~Graph();

    uint64_t id() const { return _id; }

    // MARK: Context

    Context *_Nullable context_with_id(uint64_t context_id) const {
        return _contexts_by_id.lookup(context_id, nullptr);
    }
    Context *_Nullable primary_context() const;

    inline static void retain(Graph *graph) { graph->_ref_count += 1; };
    inline static void release(Graph *graph) {
        graph->_ref_count -= 1;
        if (graph->_ref_count == 0) {
            delete graph;
        }
    };

    // MARK: Subgraphs

    vector<Subgraph *, 0, uint32_t> &subgraphs() { return _subgraphs; };

    void add_subgraph(Subgraph &subgraph);
    void remove_subgraph(Subgraph &subgraph);

    class without_invalidating {
      private:
        Graph *_graph;
        bool _was_deferring;

      public:
        without_invalidating(Graph *graph);
        ~without_invalidating();
    };

    bool is_deferring_subgraph_invalidation() const { return _deferring_subgraph_invalidation; }

    bool begin_deferring_subgraph_invalidation() {
        bool previous = _deferring_subgraph_invalidation;
        _deferring_subgraph_invalidation = true;
        return previous;
    };

    void end_deferring_subgraph_invalidation(bool was_deferring) {
        if (!was_deferring) {
            _deferring_subgraph_invalidation = false;
            invalidate_subgraphs();
        }
    };

    void defer_subgraph_invalidation(Subgraph &subgraph) { _invalidating_subgraphs.push_back(&subgraph); };

    void invalidate_subgraphs();
    void will_invalidate_subgraph() { _deferring_subgraph_invalidation = true; }
    void did_invalidate_subgraph() { _deferring_subgraph_invalidation = false; }

    // MARK: Main handler

    bool has_main_handler() const { return _main_handler != nullptr; }

    // MARK: Metrics

    uint64_t num_subgraphs() const { return _num_subgraphs; };
    uint64_t num_subgraphs_total() const { return _num_subgraphs_total; };

    // MARK: Attribute types

    const AttributeType &attribute_type(uint32_t type_id) const { return *_types[type_id]; };
    const AttributeType &attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const;

    uint32_t intern_type(const swift::metadata *metadata, ClosureFunctionVP<const AGAttributeType *> make_type);

    // MARK: Attributes

    data::ptr<Node> add_attribute(Subgraph &subgraph, uint32_t type_id, const void *body, const void *_Nullable value);
    void update_main_refs(AttributeID attribute);

    void did_allocate_node_value(size_t size);
    void did_destroy_node_value(size_t size);

    void update_attribute(AttributeID attribute, bool option);

    // MARK: Value

    bool value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value,
                            const swift::metadata &metadata);

    // MARK: Trace

    void start_tracing(AGTraceFlags trace_flags, std::span<const char *> subsystems);
    void stop_tracing();
    void sync_tracing();
    CFStringRef copy_trace_path();

    void prepare_trace(Trace &trace);

    void add_trace(Trace *_Nullable trace);
    void remove_trace(uint64_t trace_id);

    static void all_start_tracing(AGTraceFlags trace_flags, std::span<const char *> span);
    static void all_stop_tracing();
    static void all_sync_tracing();
    static CFStringRef all_copy_trace_path();

    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    const vector<Trace *, 0, uint32_t> &traces() const { return _traces; };

    template <typename T>
        requires std::invocable<T, Trace &>
    void foreach_trace(T body) {
        for (auto trace = _traces.rbegin(), end = _traces.rend(); trace != end; ++trace) {
            body(**trace);
        }
    };

    // MARK: Keys

    uint32_t intern_key(const char *key);
    const char *key_name(uint32_t key_id) const;

    // MARK: Description

#ifdef __OBJC__
    static NSObject *_Nullable description(Graph *_Nullable graph, NSDictionary *options);
    static NSDictionary *description_graph(Graph *_Nullable graph, NSDictionary *options);
#endif
};

} // namespace AG

CF_ASSUME_NONNULL_END
