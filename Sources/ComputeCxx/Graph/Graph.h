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

#include <Utilities/HashTable.h>
#include <Utilities/Heap.h>
#include <Utilities/TaggedPointer.h>

#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "Closure/ClosureFunction.h"
#include "ComputeCxx/AGTrace.h"
#include "Swift/Metadata.h"
#include "Vector/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Trace;

class Graph {
  public:
    class Context;
    class TreeElement;
    class TreeElementID;
    class TreeValue;
    class TreeValueID;
    class KeyTable;
    class UpdateStack;
    class TraceRecorder;

    class TreeDataElement {
        using TreeElementNodePair = std::pair<data::ptr<TreeElement>, data::ptr<Node>>;

      private:
        vector<TreeElementNodePair, 0, uint64_t> _nodes;
        bool _sorted;

        void sort_nodes();

      public:
        vector<TreeElementNodePair, 0, uint64_t> &nodes() {
            sort_nodes();
            return _nodes;
        };

        void push_back(TreeElementNodePair pair) { _nodes.push_back(pair); };
    };

    enum class UpdateStatus : uint32_t {
        Unchanged = 0,
        Changed = 1,
        Aborted = 2,
        NeedsCallMainHandler = 3,
    };

    typedef void (*MainHandler)(const void *_Nullable context AG_SWIFT_CONTEXT, void (*trampoline_thunk)(const void *),
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
    uint64_t _num_value_bytes = 0;

    // Trace recorder
    TraceRecorder *_trace_recorder;

    // Tree
    std::unique_ptr<std::unordered_map<Subgraph *, TreeDataElement>> _tree_data_elements_by_subgraph;
    KeyTable *_Nullable _keys;

    // Subgraphs
    vector<Subgraph *, 0, uint32_t> _subgraphs;
    vector<Subgraph *, 2, uint32_t> _invalidating_subgraphs;
    bool _deferring_subgraph_invalidation;

    // Threads
    bool _needs_update;
    uint32_t _ref_count = 1;
    pthread_t _current_update_thread = 0;

    uint64_t _id;
    uint64_t _deadline = UINT64_MAX;

    // Counters
    uint64_t _transaction_count = 0;
    uint64_t _update_count = 0;
    uint64_t _update_on_main_count = 0;
    uint64_t _change_count = 0;
    uint64_t _version = 0;

    static void all_lock() { os_unfair_lock_lock(&_all_graphs_lock); };
    static bool all_try_lock() { return os_unfair_lock_trylock(&_all_graphs_lock); };
    static void all_unlock() { os_unfair_lock_unlock(&_all_graphs_lock); };

    // Main handler

    void call_main_handler(void *context, void (*body)(void *context));

    // Attributes utility methods

    void remove_removed_input(AttributeID attribute, AttributeID input);
    bool remove_removed_output(AttributeID attribute, AttributeID output, bool option);

    void remove_input(data::ptr<Node> node, uint32_t index);
    void remove_input_edge(data::ptr<Node> node_ptr, Node &node, uint32_t index);
    void remove_all_inputs(data::ptr<Node> node);
    void all_inputs_removed(data::ptr<Node> node);

    template <typename T> void add_output_edge(data::ptr<T> node, AttributeID output);
    template <> void add_output_edge<Node>(data::ptr<Node> node, AttributeID output);
    template <> void add_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output);

    template <typename T> void remove_output_edge(data::ptr<T> node, AttributeID attribute);
    template <> void remove_output_edge<Node>(data::ptr<Node> node, AttributeID output);
    template <> void remove_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output);

    void add_input_dependencies(AttributeID attribute, AttributeID input);
    void remove_input_dependencies(AttributeID attribute, AttributeID input);
    void update_main_refs(AttributeID attribute);

    void *input_value_ref_slow(data::ptr<Node> node, AttributeID input, uint32_t subgraph_id,
                               AGInputOptions input_options, const swift::metadata &value_type,
                               AGChangedValueFlags *_Nonnull flags_out, uint32_t index);

    uint32_t index_of_input(Node &node, InputEdge::Comparator comparator);
    uint32_t index_of_input_slow(Node &node, InputEdge::Comparator comparator);

    void mark_pending(data::ptr<Node> node_ptr, Node *node);

    // Update methods

    bool compare_edge_values(InputEdge input_edge, AttributeType *_Nullable type, const void *destination_value,
                             const void *source_value);

    inline bool update_attribute_checked(data::ptr<Node> node, uint32_t subgraph_id, AGGraphUpdateOptions options,
                                         AGChangedValueFlags *_Nullable flags_out);

    static pthread_key_t _current_update_key;

    bool passed_deadline_slow();
    void collect_stack(vector<data::ptr<Node>, 0, uint64_t> &nodes);

  public:
    Graph();
    ~Graph();

    uint64_t id() const { return _id; }

    // MARK: Context

    Context *_Nullable context_with_id(uint64_t context_id) const {
        return _contexts_by_id.lookup(context_id, nullptr);
    }
    Context *_Nullable primary_context() const;

    bool is_context_updating(uint64_t context_id);

    inline static void retain(Graph *graph) { graph->_ref_count += 1; };
    inline static void release(Graph *graph) {
        graph->_ref_count -= 1;
        if (graph->_ref_count == 0) {
            delete graph;
        }
    };

    // MARK: Main handler

    bool has_main_handler() const { return _main_handler != nullptr; }

    void with_main_handler(ClosureFunctionVV<void> body, MainHandler _Nullable main_handler,
                           const void *_Nullable main_handler_context);

    // MARK: Tree

    TreeDataElement *_Nullable tree_data_element_for_subgraph(Subgraph *subgraph) {
        if (!_tree_data_elements_by_subgraph) {
            return nullptr;
        }

        auto iter = _tree_data_elements_by_subgraph->find(subgraph);
        if (iter == _tree_data_elements_by_subgraph->end()) {
            return nullptr;
        }

        return &iter->second;
    };

    void add_tree_data_for_subgraph(Subgraph *subgraph, data::ptr<TreeElement> tree_element, data::ptr<Node> node) {
        if (!_tree_data_elements_by_subgraph) {
            _tree_data_elements_by_subgraph.reset(new std::unordered_map<Subgraph *, TreeDataElement>());
        }
        auto &tree_data_element = _tree_data_elements_by_subgraph->try_emplace(subgraph).first->second;
        tree_data_element.push_back({tree_element, node});
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

    // MARK: Metrics

    uint64_t num_nodes() const { return _num_nodes; };
    uint64_t num_nodes_total() const { return _num_nodes_total; };
    uint64_t num_subgraphs() const { return _num_subgraphs; };
    uint64_t num_subgraphs_total() const { return _num_subgraphs_total; };

    // MARK: Attribute types

    const AttributeType &attribute_type(uint32_t type_id) const { return *_types[type_id]; };
    const AttributeType &attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const;

    uint32_t intern_type(const swift::metadata *metadata, ClosureFunctionVP<const AGAttributeType *> make_type);

    // MARK: Attributes

    data::ptr<Node> add_attribute(Subgraph &subgraph, uint32_t type_id, const void *body, const void *_Nullable value);
    data::ptr<IndirectNode> add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset,
                                                   std::optional<size_t> size, bool is_mutable);

    void remove_node(data::ptr<Node> node);
    void remove_indirect_node(data::ptr<IndirectNode> node);

    uint32_t add_input(data::ptr<Node> node, AttributeID input, bool allow_nil, AGInputOptions options);

    void indirect_attribute_set(data::ptr<IndirectNode> indirect_node, AttributeID source);
    bool indirect_attribute_reset(data::ptr<IndirectNode> indirect_node, bool non_nil);

    AttributeID indirect_attribute_dependency(data::ptr<IndirectNode> indirect_node);
    void indirect_attribute_set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency);

    // MARK: Search

    bool breadth_first_search(AttributeID attribute, AGSearchOptions options,
                              ClosureFunctionAB<bool, AGAttribute> predicate) const;

    // MARK: Body

    void attribute_modify(data::ptr<Node> node, const swift::metadata &type, ClosureFunctionPV<void, void *> modify,
                          bool invalidating);

    // MARK: Value

    bool value_exists(data::ptr<Node> node);
    AGValueState value_state(AttributeID attribute);

    void *value_ref(AttributeID attribute, uint32_t subgraph_id, const swift::metadata &value_type,
                    AGChangedValueFlags *_Nonnull flags_out);

    void *input_value_ref(data::ptr<Node> node, AttributeID input, uint32_t subgraph_id, AGInputOptions input_options,
                          const swift::metadata &value_type, AGChangedValueFlags *_Nonnull flags_out);

    bool value_set(data::ptr<Node> node, const swift::metadata &metadata, const void *value);
    bool value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value, const swift::metadata &metadata);

    void value_mark(data::ptr<Node> node);
    void value_mark_all();

    void propagate_dirty(AttributeID attribute);

    bool any_inputs_changed(data::ptr<Node> node, const AttributeID *exclude_attributes,
                            uint64_t exclude_attributes_count);

    void input_value_add(data::ptr<Node> node, AttributeID input, AGInputOptions options);

    void *output_value_ref(data::ptr<Node> node, const swift::metadata &value_type);

    void did_allocate_value(size_t size) { _num_value_bytes += size; };
    void did_destroy_value(size_t size) { _num_value_bytes -= size; };
    void did_destroy_node() { _num_nodes -= 1; };

    // MARK: Update

    static util::tagged_ptr<UpdateStack> current_update() {
        return util::tagged_ptr<UpdateStack>((UpdateStack *)pthread_getspecific(_current_update_key));
    }

    static void set_current_update(util::tagged_ptr<UpdateStack> current_update) {
        pthread_setspecific(_current_update_key, (void *)current_update.value());
    }

    void set_deadline(uint64_t deadline) { _deadline = deadline; };
    bool passed_deadline();

    bool thread_is_updating();

    uint64_t transaction_count() const { return _transaction_count; };
    void increment_transaction_count_if_needed() {
        if (!thread_is_updating()) {
            _transaction_count += 1;
        }
    };

    uint64_t version() const { return _version; }

    bool needs_update() { return _needs_update; };
    void set_needs_update(bool needs_update) { _needs_update = needs_update; };

    void call_update();

    void with_update(data::ptr<Node> node, ClosureFunctionVV<void> body);
    static void without_update(ClosureFunctionVV<void> body);

    UpdateStatus update_attribute(data::ptr<Node> node, AGGraphUpdateOptions options);
    void reset_update(data::ptr<Node> node);

    void mark_changed(data::ptr<Node> node, AttributeType *_Nullable type, const void *_Nullable destination_value,
                      const void *_Nullable source_value);
    void mark_changed(AttributeID attribute, AttributeType *_Nullable type, const void *_Nullable destination_value,
                      const void *_Nullable source_value, uint32_t start_output_index);

    static void compare_failed(const void *lhs, const void *rhs, size_t range_offset, size_t range_size,
                               const swift::metadata *_Nullable type);

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
