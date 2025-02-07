#pragma once

#include <CoreFoundation/CFBase.h>
#include <concepts>

#include "Attribute/AttributeID.h"
#include "Attribute/Node/Node.h"
#include "Closure/ClosureFunction.h"
#include "Containers/IndirectPointerVector.h"
#include "Data/Pointer.h"
#include "Data/Zone.h"
#include "Graph/Graph.h"
#include "Private/CFRuntime.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
}
class Graph;
class Node;
class Encoder;

class SubgraphObject {
  private:
    CFRuntimeBase _base;
    Subgraph *_subgraph;

  public:
    Subgraph *subgraph() { return _subgraph; };
    void clear_subgraph() { _subgraph = nullptr; };
};

class Subgraph : public data::zone {
  public:
    class NodeCache;
    class SubgraphChild {
      private:
        uintptr_t _data;

      public:
        enum Flags : uint32_t {};
        SubgraphChild(Subgraph *subgraph, Flags flags) { _data = (uintptr_t)subgraph | (flags & 0x3); };
        Subgraph *subgraph() const { return reinterpret_cast<Subgraph *>(_data & ~0x3); };
        bool operator==(const Subgraph *other) const { return subgraph() == other; };
    };

    struct Flags {
        uint8_t value1;
        uint8_t value2;
        uint8_t value3;
        uint8_t value4;
        bool is_null() const { return value1 == 0 && value2 == 0 and value3 == 0 && value4 == 0; };
    };

    struct Observer {};

    enum CacheState : uint8_t {
        Option1 = 1 << 0, // added to graph._subgraphs_with_cached_nodes, or needs collect?
        Option2 = 1 << 1, // Is calling cache collect
    };

    enum ValidationState : uint8_t {
        Valid = 0,
        InvalidationScheduled = 1,
        Invalidated = 2,
        GraphDestroyed = 3,
    };

  private:
    static pthread_key_t _current_subgraph_key;

    SubgraphObject *_object;
    Graph *_Nullable _graph;
    uint64_t _graph_context_id;

    indirect_pointer_vector<Subgraph> _parents;
    vector<SubgraphChild, 0, uint32_t> _children;

    using observers_vector = vector<std::pair<ClosureFunctionVV<void>, uint64_t>, 0, uint64_t>;
    data::ptr<observers_vector> _observers;
    uint32_t _traversal_seed;

    uint32_t _index; // TODO: get and set

    data::ptr<NodeCache> _cache;
    data::ptr<Graph::TreeElement> _tree_root;

    Flags _flags;
    ValidationState _validation_state;
    uint8_t _other_state;

  public:
    static void make_current_subgraph_key();
    static Subgraph *_Nullable current_subgraph();
    static void set_current_subgraph(Subgraph *_Nullable subgraph);

    Subgraph(SubgraphObject *object, Graph::Context &context, AttributeID attribute);
    ~Subgraph();

    // MARK: CoreFoundation

    static Subgraph *from_cf(SubgraphObject *object);
    SubgraphObject *to_cf();
    void clear_object();

    // MARK: Graph

    Graph *_Nullable graph() const { return _graph; };
    uint64_t graph_context_id() { return _graph_context_id; };

    bool is_valid() { return _validation_state == ValidationState::Valid; };
    ValidationState validation_state() { return _validation_state; };

    uint8_t other_state() { return _other_state; };
    void set_other_state(uint8_t other_state) { _other_state = other_state; };

    void invalidate_and_delete_(bool delete_subgraph);
    void invalidate_now(Graph &graph);
    void graph_destroyed();

    // MARK: Managing children

    void add_child(Subgraph &child, SubgraphChild::Flags flags);
    void remove_child(Subgraph &child, bool flag);

    bool ancestor_of(const Subgraph &other);

    template <typename Callable>
        requires std::invocable<Callable, Subgraph &> && std::same_as<std::invoke_result_t<Callable, Subgraph &>, bool>
    void foreach_ancestor(Callable body);

    // MARK: Attributes

    void add_node(data::ptr<Node> node);
    void add_indirect(data::ptr<IndirectNode> node, bool flag);

    void insert_attribute(AttributeID attribute, bool dirty);

    void unlink_attribute(AttributeID attribute);

    void update(uint8_t flags);

    // MARK: Traversal

    static std::atomic<uint32_t> _last_traversal_seed;

    void apply(Flags flags, ClosureFunctionAV<void, unsigned int> body);

    // MARK: Tree

    void begin_tree(AttributeID attribute, const swift::metadata *_Nullable type,
                    uint32_t flags); // TODO: check can be null from Subgraph()
    void end_tree(AttributeID attribute);

    void set_tree_owner(AttributeID owner);
    void add_tree_value(AttributeID attribute, const swift::metadata *type, const char *value, uint32_t flags);

    AttributeID tree_node_at_index(data::ptr<Graph::TreeElement> tree_element, uint64_t index);
    uint32_t tree_subgraph_child(data::ptr<Graph::TreeElement> tree_element);

    // MARK: Managing flags

    // flags 2 and 4 control propogation
    // flags 1 and 3 are the values themselvs
    // flags 3 and 4 are the dirty subset of 1 and 2

    void set_flags(data::ptr<Node> node, NodeFlags::Flags3 flags3);

    void add_flags(uint8_t flags);
    void add_dirty_flags(uint8_t dirty_flags);

    void propagate_flags();
    void propagate_dirty_flags();

    // MARK: Managing observers

    uint64_t add_observer(ClosureFunctionVV<void> observer);
    void remove_observer(uint64_t observer_id);
    void notify_observers();

    // MARK: Cache

    data::ptr<Node> cache_fetch(uint64_t identifier, const swift::metadata &type, void *body,
                                ClosureFunctionCI<unsigned long, Graph *> closure);
    void cache_insert(data::ptr<Node> node);
    void cache_collect();

    // MARK: Encoding

    void encode(Encoder &encoder);

    // MARK: Printing

    void print(uint32_t indent_level);
};

} // namespace AG

CF_ASSUME_NONNULL_END
