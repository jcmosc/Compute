#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSubgraph-Private.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "Closure/ClosureFunction.h"
#include "Data/Zone.h"
#include "Graph/Graph.h"

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
  private:
    static pthread_key_t _current_subgraph_key;

    SubgraphObject *_object;
    Graph *_graph;
    uint64_t _context_id;

    struct Observer {
        ClosureFunctionVV<void> callback;
        uint64_t observer_id;
    };
    data::ptr<vector<Observer, 0, uint64_t> *> _observers;

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

    void invalidate_and_delete_(bool delete_subgraph);
};

} // namespace AG

CF_ASSUME_NONNULL_END
