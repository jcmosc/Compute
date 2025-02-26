#pragma once

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <span>
#include <stdint.h>
#include <unordered_map>

#include "Attribute/AttributeID.h"
#include "Attribute/Node/Edge.h"
#include "Closure/ClosureFunction.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"
#include "Utilities/TaggedPointer.h"

CF_ASSUME_NONNULL_BEGIN

typedef uint8_t AGValueState; // TODO: move

namespace AG {

namespace swift {
class metadata;
}
class AttributeType;
class Subgraph;
class Encoder;
class Trace;
class MutableIndirectNode;

class Graph {
  public:
    class Context;
    class KeyTable;
    class TraceRecorder;
    class UpdateStack;
    class UpdateStackRef;
    class ProfileData;
    class ProfileTrace;

    enum SearchOptions : uint32_t {
        SearchInputs = 1 << 0,
        SearchOutputs = 1 << 1,
        TraverseGraphContexts = 1 << 2,
    };

    enum UpdateStatus : uint32_t {
        NoChange = 0,
        Changed = 1,
        Option2 = 2,
        NeedsCallMainHandler = 3,
    };

    struct TreeElement;
    struct TreeValue;
    class TreeDataElement {
        using TreeElementNodePair = std::pair<data::ptr<Graph::TreeElement>, data::ptr<Node>>;

      private:
        vector<TreeElementNodePair, 0, uint64_t> _nodes;
        bool _sorted;

      public:
        vector<TreeElementNodePair, 0, uint64_t> nodes() { return _nodes; };
        void sort_nodes();
        void push_back(TreeElementNodePair pair) { _nodes.push_back(pair); };
    };

    using MainHandler = void (*)(void (*thunk)(void *), void *thunk_context); // needs AG_SWIFT_CC(swift) ?

  private:
    static Graph *_all_graphs;
    static os_unfair_lock _all_graphs_lock;
    static pthread_key_t _current_update_key;

    // Graph
    Graph *_next;
    Graph *_previous;
    util::Heap _heap;

    static void all_lock() { os_unfair_lock_lock(&_all_graphs_lock); };
    static bool all_try_lock() { return os_unfair_lock_trylock(&_all_graphs_lock); };
    static void all_unlock() { os_unfair_lock_unlock(&_all_graphs_lock); };

    // Attribute types
    util::UntypedTable _type_ids_by_metadata;
    vector<AttributeType *, 0, uint32_t> _types;

    // Contexts
    util::Table<uint64_t, Context *> _contexts_by_id;

    // Traces
    vector<Trace *, 0, uint32_t> _traces;

    // Main thread handler
    MainHandler _Nullable _main_handler;
    void *_Nullable _main_handler_context;

    // Metrics
    uint64_t _num_nodes = 0;
    uint64_t _num_nodes_created = 0;
    uint64_t _num_subgraphs = 0;
    uint64_t _num_subgraphs_created = 0;
    size_t _num_node_value_bytes = 0;

    // Profile
    bool _is_profiling;
    std::unique_ptr<ProfileData> _profile_data;
    ProfileTrace *_Nullable _profile_trace;

    // Trace
    TraceRecorder *_trace_recorder;

    // Tree
    std::unique_ptr<std::unordered_map<Subgraph *, TreeDataElement>> _tree_data_elements_by_subgraph;
    KeyTable *_Nullable _keys;

    // Subgraphs
    vector<Subgraph *, 0, uint32_t> _subgraphs;
    vector<Subgraph *, 0, uint32_t> _subgraphs_with_cached_nodes;
    vector<Subgraph *, 2, uint32_t> _invalidated_subgraphs;
    bool _batch_invalidate_subgraphs;

    // Updates
    bool _needs_update;
    uint32_t _num_contexts;
    pthread_t _current_update_thread = 0;

    uint64_t _unique_id;
    uint64_t _deadline = UINT64_MAX;

    // Counters
    uint64_t _transaction_count = 0;
    uint64_t _update_count = 0;
    uint64_t _update_on_main_count = 0;
    uint64_t _change_count = 0;
    uint64_t _version = 0;

  public:
    Graph();
    ~Graph();

    // MARK: Context

    bool is_context_updating(uint64_t context_id);
    Context *_Nullable main_context() const;

    // MARK: Subgraphs

    class without_invalidating {
      private:
        Graph *_graph;
        bool _graph_old_batch_invalidate_subgraphs;

      public:
        without_invalidating(Graph *graph);
        ~without_invalidating();
    };

    vector<Subgraph *, 0, uint32_t> &subgraphs() { return _subgraphs; };

    void add_subgraph(Subgraph &subgraph); // called from constructor of Subgraph
    void remove_subgraph(Subgraph &subgraph);

    void will_invalidate_subgraph(Subgraph &subgraph) { _invalidated_subgraphs.push_back(&subgraph); };

    void invalidate_subgraphs();
    bool batch_invalidate_subgraphs() { return _batch_invalidate_subgraphs; };
    void set_batch_invalidate_subgraphs(bool value) { _batch_invalidate_subgraphs = value; };

    void remove_subgraphs_with_cached_node(Subgraph *subgraph); // overload with iter?
    void add_subgraphs_with_cached_node(Subgraph *subgraph) { _subgraphs_with_cached_nodes.push_back(subgraph); }

    // MARK: Updates

    static util::tagged_ptr<UpdateStack> current_update();
    static void set_current_update(util::tagged_ptr<UpdateStack> current_update);

    bool thread_is_updating();

    bool needs_update() { return _needs_update; };
    void set_needs_update(bool needs_update) { _needs_update = needs_update; };

    void call_update();
    void reset_update(data::ptr<Node> node);

    void collect_stack(vector<data::ptr<Node>, 0, uint64_t> &nodes);

    void with_update(data::ptr<AG::Node> node, ClosureFunctionVV<void> body);

    MainHandler _Nullable main_handler() { return _main_handler; };
    void call_main_handler(void *context, void (*body)(void *context));

    void set_deadline(uint64_t deadline) { _deadline = deadline; };
    bool passed_deadline();
    bool passed_deadline_slow();

    void increment_transaction_count_if_needed() {
        if (!thread_is_updating()) {
            _transaction_count += 1;
        }
    };

    uint64_t unique_id() const { return _unique_id; };

    // MARK: Attributes

    const AttributeType &attribute_type(uint32_t type_id) const;
    const AttributeType &attribute_ref(data::ptr<Node> node, const void *_Nullable *_Nullable self_out) const;

    void attribute_modify(data::ptr<Node> node, const swift::metadata &type, ClosureFunctionPV<void, void *> closure,
                          bool flag);

    data::ptr<Node> add_attribute(Subgraph &subgraph, uint32_t type_id, void *body, void *_Nullable value);

    UpdateStatus update_attribute(AttributeID attribute, uint8_t options);
    void update_main_refs(AttributeID attribute);

    void remove_node(data::ptr<Node> node);

    bool breadth_first_search(AttributeID attribute, SearchOptions options,
                              ClosureFunctionAB<bool, uint32_t> predicate) const;

    void did_allocate_node_value(size_t size) { _num_node_value_bytes += size; };
    void did_destroy_node_value(size_t size) { _num_node_value_bytes -= size; };

    void did_destroy_node() { _num_nodes -= 1; };

    // MARK: Indirect attributes

    void add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset, std::optional<size_t> size,
                                bool is_mutable);
    void remove_indirect_node(data::ptr<IndirectNode> node);

    void indirect_attribute_set(data::ptr<IndirectNode>, AttributeID source);
    bool indirect_attribute_reset(data::ptr<IndirectNode>, bool non_nil);

    const AttributeID &indirect_attribute_dependency(data::ptr<IndirectNode> indirect_node);
    void indirect_attribute_set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency);

    // MARK: Values

    void *value_ref(AttributeID attribute, bool evaluate_weak_references, const swift::metadata &value_type,
                    bool *_Nonnull did_update_out);

    bool value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value);
    bool value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value, const swift::metadata &type);

    bool value_exists(data::ptr<Node> node);
    AGValueState value_state(AttributeID attribute);

    void value_mark(data::ptr<Node> node);
    void value_mark_all();

    void propagate_dirty(AttributeID attribute);

    // MARK: Inputs

    void *_Nullable input_value_ref_slow(data::ptr<AG::Node> node, AttributeID input, bool evaluate_weak_references,
                                         uint8_t input_flags, const swift::metadata &type, char *arg5, uint32_t index);

    void input_value_add(data::ptr<Node> node, AttributeID input, uint8_t input_edge_flags);

    uint32_t add_input(data::ptr<Node> node, AttributeID input, bool allow_nil, uint8_t input_edge_flags);
    void remove_input(data::ptr<Node> node, uint32_t index);
    void remove_all_inputs(data::ptr<Node> node);

    void add_input_dependencies(AttributeID attribute, AttributeID input);
    void remove_input_dependencies(AttributeID attribute, AttributeID input);

    void remove_input_edge(data::ptr<Node> node_ptr, Node &node, uint32_t index);

    void remove_removed_input(AttributeID attribute, AttributeID input);

    bool any_inputs_changed(data::ptr<Node> node, const AttributeID *attributes, uint64_t count);
    void all_inputs_removed(data::ptr<Node> node);

    uint32_t index_of_input(Node &node, InputEdge::Comparator comparator);
    uint32_t index_of_input_slow(Node &node, InputEdge::Comparator comparator);

    bool compare_edge_values(InputEdge input_edge, const AttributeType *_Nullable type, const void *destination_value,
                             const void *source_value);

    // MARK: Outputs

    void *output_value_ref(data::ptr<Node> node, const swift::metadata &type);

    template <typename T> void add_output_edge(data::ptr<T> node, AttributeID output);
    template <> void add_output_edge<Node>(data::ptr<Node> node, AttributeID output);
    template <> void add_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output);

    template <typename T> void remove_output_edge(data::ptr<T> node, AttributeID attribute);
    template <> void remove_output_edge<Node>(data::ptr<Node> node, AttributeID attribute);
    template <>
    void remove_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID attribute);

    bool remove_removed_output(AttributeID attribute, AttributeID source, bool flag);

    // MARK: Marks

    void mark_changed(data::ptr<Node> node, AttributeType *_Nullable type, const void *_Nullable destination_value,
                      const void *_Nullable source_value);
    void mark_changed(AttributeID attribute, AttributeType *_Nullable type, const void *_Nullable destination_value,
                      const void *_Nullable source_value, uint32_t start_output_index);

    void mark_pending(data::ptr<Node> node_ptr, Node *node);

    Graph::TreeDataElement &tree_data_element_for_subgraph(Subgraph *subgraph) {
        if (!_tree_data_elements_by_subgraph) {
            _tree_data_elements_by_subgraph.reset(new std::unordered_map<Subgraph *, TreeDataElement>());
        }
        return _tree_data_elements_by_subgraph->try_emplace(subgraph).first->second;
    };

    std::unordered_map<Subgraph *, TreeDataElement> *tree_data_elements() {
        return _tree_data_elements_by_subgraph.get();
    };

    // MARK: Intern

    uint32_t intern_key(const char *key);
    const char *key_name(uint32_t key_id) const;

    uint32_t intern_type(swift::metadata *metadata, ClosureFunctionVP<void *> make_type);

    // MARK: Encoding

    void encode_node(Encoder &encoder, const Node &node, bool flag);
    void encode_indirect_node(Encoder &encoder, const IndirectNode &indirect_node);

    void encode_tree(Encoder &encoder, data::ptr<TreeElement> tree);

    // MARK: Counters

    // Counters
    uint64_t transaction_count() const { return _transaction_count; };
    uint64_t update_count() const { return _update_count; };
    uint64_t change_count() const { return _change_count; };

    // MARK: Tracing

    enum TracingFlags : uint32_t {
        Enabled = 1 << 0,
        Full = 1 << 1,
        Backtrace = 1 << 2,
        Prepare = 1 << 3,
        Custom = 1 << 4, // skip updates
        All = 1 << 5,
    };

    void prepare_trace(Trace &trace);

    void add_trace(Trace *_Nullable trace);
    void remove_trace(uint64_t trace_id);

    void start_tracing(uint32_t tracing_flags, std::span<const char *> subsystems);
    void stop_tracing();
    void sync_tracing();
    CFStringRef copy_trace_path();

    template <typename T>
        requires std::invocable<T, Trace &>
    void foreach_trace(T body) {
        for (auto trace = _traces.rbegin(), end = _traces.rend(); trace != end; ++trace) {
            body(**trace);
        }
    };

    static void all_start_tracing(uint32_t tracing_flags, std::span<const char *, UINT64_MAX> span);
    static void all_stop_tracing();
    static void all_sync_tracing();
    static CFStringRef all_copy_trace_path();

    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    // MARK: Profile

    bool is_profiling() const { return _is_profiling; };

    uint64_t begin_profile_event(data::ptr<Node> node, const char *event_name);
    void end_profile_event(data::ptr<Node> node, const char *event_name, uint64_t arg1, bool arg2);

    void add_profile_update(data::ptr<Node> node, uint64_t duration, bool changed);

    void start_profiling(uint32_t profiler_flags);
    void stop_profiling();
    void mark_profile(uint32_t event_id, uint64_t time);
    void reset_profile();

    static void all_start_profiling(uint32_t profiler_flags);
    static void all_stop_profiling();
    static void all_mark_profile(const char *name);
    static void all_reset_profile();

    // MARK: Printing

    void print();
    void print_attribute(data::ptr<Node> node);

    void print_cycle(data::ptr<Node> node);

    void print_data();
    static void print_stack();

    // MARK: Description

    CFStringRef description(data::ptr<Node> node);

    static id description(Graph *_Nullable graph, CFDictionaryRef options);
    static CFDictionaryRef description_graph(Graph *graph, CFDictionaryRef options);
    CFStringRef description_graph_dot(CFDictionaryRef _Nullable options);
    CFStringRef description_stack(CFDictionaryRef options);
    CFArrayRef description_stack_nodes(CFDictionaryRef options);
    CFDictionaryRef description_stack_frame(CFDictionaryRef options);

    static void write_to_file(Graph *_Nullable graph, const char *_Nullable filename, bool exclude_values);
};

} // namespace AG

CF_ASSUME_NONNULL_END
