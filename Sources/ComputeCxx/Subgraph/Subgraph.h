#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSubgraph-Private.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "Closure/ClosureFunction.h"
#include "Data/Zone.h"
#include "Graph/Graph.h"
#include "Vector/IndirectPointerVector.h"

CF_ASSUME_NONNULL_BEGIN

struct AGSubgraphStorage;

namespace AG {

class SubgraphObject {
  private:
    AGSubgraphStorage _storage;

  public:
    Subgraph *subgraph() { return _storage.subgraph; };
    void clear_subgraph() { _storage.subgraph = nullptr; };
};

class Subgraph : public data::zone {
  public:
    class SubgraphChild {
      private:
        enum : uintptr_t {
            mask = 0x3,
        };

        uintptr_t _data;

      public:
        SubgraphChild(Subgraph *subgraph, uint8_t flags) { _data = (uintptr_t)subgraph | (flags & mask); };

        Subgraph *subgraph() const { return reinterpret_cast<Subgraph *>(_data & ~mask); };
        uint8_t flags() const { return _data & mask; };
    };

  private:
    static pthread_key_t _current_subgraph_key;

    SubgraphObject *_object;
    Graph *_graph;
    uint64_t _context_id;

    indirect_pointer_vector<Subgraph> _parents;
    vector<SubgraphChild, 0, uint32_t> _children;

    struct Observer {
        ClosureFunctionVV<void> callback;
        uint64_t observer_id;
    };
    data::ptr<vector<Observer, 0, uint64_t> *> _observers;

    enum class InvalidationState : uint8_t {
        None = 0,
        Deferred = 1,
        Completed = 2,
        GraphDestroyed = 3,
    };

    InvalidationState _invalidation_state = InvalidationState::None;

  public:
    Subgraph(SubgraphObject *object, Graph::Context &context, AttributeID attribute);
    ~Subgraph();

    // MARK: CFType

    AGSubgraphStorage *to_cf() const;
    static Subgraph *from_cf(AGSubgraphStorage *storage);
    void clear_object();

    // MARK: Current subgraph

    static void make_current_subgraph_key();
    static Subgraph *_Nullable current_subgraph();
    static void set_current_subgraph(Subgraph *_Nullable subgraph);

    // MARK: Observers

    uint64_t add_observer(ClosureFunctionVV<void> callback);
    void remove_observer(uint64_t observer_id);
    void notify_observers();

    // MARK: Graph

    Graph *graph() const { return _graph; };
    uint64_t context_id() const { return _context_id; }

    bool is_valid() const { return _invalidation_state == InvalidationState::None; }
    bool is_invalidating() const {
        return _invalidation_state >= InvalidationState::Deferred &&
               _invalidation_state <= InvalidationState::GraphDestroyed;
    }
    void invalidate_and_delete_(bool delete_zone_data);
    void invalidate_deferred(Graph &graph);
    void invalidate_now(Graph &graph);
    void graph_destroyed();

    // MARK: Children

    indirect_pointer_vector<Subgraph> &parents() { return _parents; };
    vector<SubgraphChild, 0, uint32_t> &children() { return _children; };

    void add_child(Subgraph &child, uint8_t flags);
    void remove_child(Subgraph &child, bool suppress_trace);

    bool ancestor_of(const Subgraph &other);

    template <typename Callable>
        requires std::invocable<Callable, Subgraph &> && std::same_as<std::invoke_result_t<Callable, Subgraph &>, bool>
    void foreach_ancestor(Callable body) {
        for (auto iter = _parents.rbegin(), end = _parents.rend(); iter != end; ++iter) {
            auto parent = *iter;
            if (body(*parent)) {
                parent->foreach_ancestor(body);
            }
        }
    }
};

} // namespace AG

CF_ASSUME_NONNULL_END
