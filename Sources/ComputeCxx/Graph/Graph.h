#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>
#include <unordered_map>

#include "Attribute/AttributeID.h"
#include "Closure/ClosureFunction.h"
#include "Util/HashTable.h"
#include "Util/Heap.h"

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

class Graph {
  public:
    class Context;
    class KeyTable;
    class UpdateStack;
    enum class UpdateStatus;

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

  private:
    Graph *_prev;
    Graph *_next;
    util::Heap _heap;
    util::UntypedTable _type_ids_by_metadata;
    vector<AttributeType *, 0, uint32_t> _types;

    vector<Trace *, 0, uint32_t> _traces;
    uint64_t _field_0xf0;

    size_t _allocated_node_values_size = 0;

    std::unique_ptr<std::unordered_map<Subgraph *, TreeDataElement>> _tree_data_elements_by_subgraph;
    KeyTable *_Nullable _keys;

    // TODO: check field offets
    vector<Subgraph *, 0, uint32_t> _subgraphs;
    vector<Subgraph *, 0, uint32_t> _subgraphs_with_cached_nodes;
    vector<Subgraph *, 2, uint32_t> _invalidated_subgraphs; // TODO: check is 2 stack length

    // TODO: check field offets
    uint64_t _num_nodes;       // probably this, not sure
    uint64_t _num_node_values; // probably this, not sure

    bool _needs_update; // 0x199

    uint64_t _counter_0x1b8;

  public:
    bool needs_update() { return _needs_update; };
    void call_update();

    bool thread_is_updating();

    void increment_counter_0x1b8() { _counter_0x1b8 += 1; };

    // MARK: Subgraphs

    bool is_validating_0x198();
    void set_validating_0x198(bool value);

    bool field_0xf0();

    void invalidate_subgraphs();
    void will_invalidate_subgraph(Subgraph &subgraph) { _invalidated_subgraphs.push_back(&subgraph); };

    vector<Subgraph *, 0, uint32_t> &subgraphs();
    void remove_subgraph(const Subgraph &subgraph);
    
    void add_subgraphs_with_cached_node(Subgraph *subgraph) {
        _subgraphs_with_cached_nodes.push_back(subgraph);
    }

    // MARK: Attributes

    const AttributeType &attribute_type(uint32_t type_id) const;
    const AttributeType &attribute_ref(data::ptr<Node> node, const void *_Nullable *_Nullable ref_out) const;

    void attribute_modify(data::ptr<Node> node, const swift::metadata &type, ClosureFunctionPV<void, void *> closure,
                          bool flag);

    data::ptr<Node> add_attribute(Subgraph &subgraph, uint32_t type_id, void *body, void *_Nullable value);
    void remove_node(data::ptr<Node> node);

    void update_attribute(AttributeID attribute, bool option);

    void did_allocate_node_value(size_t size) { _allocated_node_values_size += size; };
    void did_destroy_node_value(size_t size) { _allocated_node_values_size -= size; };

    void did_destroy_node(); // decrement counter 0x100

    // MARK: Indirect attributes

    void add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset, std::optional<size_t> size,
                                bool is_mutable);
    void remove_indirect_node(data::ptr<IndirectNode> node);

    void indirect_attribute_set(data::ptr<IndirectNode>, AttributeID source);
    void indirect_attribute_reset(data::ptr<IndirectNode>, bool flag);

    const AttributeID &indirect_attribute_dependency(data::ptr<IndirectNode> indirect_node);
    void indirect_attribute_set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency);

    // MARK: Edges

    void add_input(data::ptr<Node> node, AttributeID attribute, bool flag, uint32_t option);
    void add_input_dependencies(AttributeID attribute, AttributeID source);
    void remove_all_inputs(data::ptr<Node> node);

    void any_inputs_changed(data::ptr<Node> node, const uint32_t *arg1, uint64_t arg2);
    void all_inputs_removed(data::ptr<Node> node);

    template <typename T> void add_output_edge(data::ptr<T> node, AttributeID attribute);
    template <typename T> void remove_output_edge(data::ptr<T> node, AttributeID attribute);

    // MARK: Values

    bool value_exists(data::ptr<Node> node);
    AGValueState value_state(AttributeID attribute);

    void value_mark(data::ptr<Node> node);
    void value_mark_all();

    bool value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value);
    bool value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value, const swift::metadata &type);

    void propagate_dirty(AttributeID attribute);

    // MARK: Marks

    void mark_changed(data::ptr<Node> node, AttributeType *_Nullable type, const void *_Nullable destination_value,
                      const void *_Nullable source_value);
    void mark_changed(AttributeID attribute, AttributeType &type, const void *_Nullable destination_value,
                      const void *_Nullable source_value, uint64_t option);

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
    const char *key_name(uint32_t key_id);

    uint32_t intern_type(swift::metadata *metadata, ClosureFunctionVP<void *> make_type);

    // MARK: Tracing

    void add_trace(Trace *_Nullable trace);
    void remove_trace(uint64_t trace_id);

    template <typename T>
        requires std::invocable<T, Trace &>
    void foreach_trace(T body) {
        for (auto trace = _traces.rbegin(), end = _traces.rend(); trace != end; ++trace) {
            body(**trace);
        }
    };

    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    // MARK: Encoding

    void encode_node(Encoder &encoder, const Node &node, bool flag);
    void encode_indirect_node(Encoder &encoder, const IndirectNode &indirect_node);

    void encode_tree(Encoder &encoder, data::ptr<TreeElement> tree);

    // MARK: Printing

    void print_data();
};

} // namespace AG

CF_ASSUME_NONNULL_END
