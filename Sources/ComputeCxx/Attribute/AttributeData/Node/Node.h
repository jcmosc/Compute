#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeData/Edge/InputEdge.h"
#include "Attribute/AttributeData/Edge/OutputEdge.h"
#include "Attribute/AttributeID/RelativeAttributeID.h"
#include "ComputeCxx/AGAttribute.h"
#include "Data/Pointer.h"
#include "Data/Vector.h"
#include "Graph/AGGraph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace data {
class zone;
}
class AttributeType;
class Graph;

enum class NodeState : uint8_t {
    Dirty = 1 << 0,
    Pending = 1 << 1,
    MainThread = 1 << 2,
    RequiresMainThread = 1 << 3,
    ValueInitialized = 1 << 4,
    SelfInitialized = 1 << 5,
    Updating = 1 << 6,
    UpdatingCyclic = 1 << 7,
};
inline NodeState operator|(NodeState a, NodeState b) {
    return static_cast<NodeState>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline NodeState operator&(NodeState a, NodeState b) {
    return static_cast<NodeState>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline NodeState operator~(NodeState a) { return static_cast<NodeState>(~static_cast<uint8_t>(a)); }

class Node {
  private:
    NodeState _state;

    // Attribute type
    unsigned int _type_id : 24 = 0;

    // Subgraph
    RelativeAttributeID _next_attribute;
    AGAttributeFlags _subgraph_flags = AGAttributeFlags(0);

    // Data flags
    unsigned int _has_indirect_self : 1 = 0;
    unsigned int _has_indirect_value : 1 = 0;
    unsigned int _input_edges_traverse_contexts : 1 = 0;
    unsigned int _needs_sort_input_edges : 1 = 0;
    unsigned int _cacheable : 1 = 0;
    unsigned int _main_ref : 1 = 0;
    unsigned int _self_modified : 1 = 0;

    // Data
    data::ptr<void> _value;
    data::vector<InputEdge> _input_edges;
    data::vector<OutputEdge> _output_edges;

  public:
    Node(uint32_t type_id, bool main_thread)
        : _type_id(type_id),
          _state(main_thread ? NodeState::MainThread | NodeState::RequiresMainThread : (NodeState)0) {};

    // Non-copyable
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    // Non-movable
    Node(Node &&) = delete;
    Node &operator=(Node &&) = delete;

    NodeState state() const { return _state; }

    bool is_dirty() const { return (_state & NodeState::Dirty) != (NodeState)0; }
    void set_dirty(bool value) { _state = value ? _state | NodeState::Dirty : _state & ~NodeState::Dirty; }

    bool is_pending() const { return (_state & NodeState::Pending) != (NodeState)0; }
    void set_pending(bool value) { _state = value ? _state | NodeState::Pending : _state & ~NodeState::Pending; }

    bool is_main_thread() const { return (_state & NodeState::MainThread) != (NodeState)0; }
    void set_main_thread(bool value) {
        _state = value ? _state | NodeState::MainThread : _state & ~NodeState::MainThread;
    }

    bool requires_main_thread() const { return (_state & NodeState::RequiresMainThread) != (NodeState)0; }
    void set_requires_main_thread(bool value) {
        _state = value ? _state | NodeState::RequiresMainThread : _state & ~NodeState::RequiresMainThread;
    }

    bool is_value_initialized() const { return (_state & NodeState::ValueInitialized) != (NodeState)0; }
    void set_value_initialized(bool value) {
        _state = value ? _state | NodeState::ValueInitialized : _state & ~NodeState::ValueInitialized;
    }

    bool is_self_initialized() const { return (_state & NodeState::SelfInitialized) != (NodeState)0; }
    void set_self_initialized(bool value) {
        _state = value ? _state | NodeState::SelfInitialized : _state & ~NodeState::SelfInitialized;
    }

    bool is_updating() const { return (_state & (NodeState::Updating | NodeState::UpdatingCyclic)) != (NodeState)0; };
    void set_updating(bool value) { _state = value ? _state | NodeState::Updating : _state & ~NodeState::Updating; }
    uint8_t count() const { return (uint8_t)_state >> 6; };

    // TODO: test this
    AGValueState flags() const {
        return (is_dirty() ? AGValueStateDirty : 0) | (is_pending() ? AGValueStatePending : 0) |
               (is_updating() ? AGValueStateUpdating : 0) | (is_value_initialized() ? AGValueStateValueExists : 0) |
               (is_main_thread() ? AGValueStateMainThread : 0) | (_main_ref ? AGValueStateMainRef : 0) |
               (requires_main_thread() ? AGValueStateRequiresMainThread : 0) |
               (_self_modified ? AGValueStateSelfModified : 0);
    };

    uint32_t type_id() const { return _type_id; };

    const RelativeAttributeID next_attribute() const { return _next_attribute; }
    void set_next_attribute(RelativeAttributeID next_attribute) { _next_attribute = next_attribute; }

    AGAttributeFlags subgraph_flags() const { return _subgraph_flags; };
    void set_subgraph_flags(AGAttributeFlags subgraph_flags) { _subgraph_flags = subgraph_flags; };

    // MARK: Data

    bool has_indirect_self() const { return _has_indirect_self; }
    void set_has_indirect_self(bool value) { _has_indirect_self = value; }

    bool has_indirect_value() const { return _has_indirect_value; }
    void set_has_indirect_value(bool value) { _has_indirect_value = value; }

    bool input_edges_traverse_contexts() const { return _input_edges_traverse_contexts; }
    void set_input_edges_traverse_contexts(bool value) { _input_edges_traverse_contexts = value; }

    void set_needs_sort_input_edges(bool value) { _needs_sort_input_edges = value; }
    void sort_input_edges_if_needed() {
        if (_needs_sort_input_edges) {
            _needs_sort_input_edges = 0;
            std::sort(_input_edges.begin(), _input_edges.end());
        }
    }

    bool is_main_ref() const { return _main_ref; }
    void set_main_ref(bool value) { _main_ref = value; }

    bool is_self_modified() const { return _self_modified; }
    void set_self_modified(bool value) { _self_modified = value; }

    data::vector<InputEdge> &input_edges() { return _input_edges; };
    uint32_t insert_input_edge(data::zone *subgraph, InputEdge &input_edge) {
        if (_needs_sort_input_edges) {
            _input_edges.push_back(subgraph, input_edge);
            return _input_edges.size() - 1;
        } else {
            auto pos = std::lower_bound(_input_edges.begin(), _input_edges.end(), input_edge);
            auto inserted = _input_edges.insert(subgraph, pos, input_edge);
            return (uint32_t)(inserted - _input_edges.begin());
        }
    }
    void remove_input_edge(uint32_t index) { _input_edges.erase(_input_edges.begin() + index); }

    data::vector<OutputEdge> &output_edges() { return _output_edges; };

    void *get_self(const AttributeType &type) const;
    void update_self(const Graph &graph, void *new_self);
    void destroy_self(const Graph &graph);

    void *get_value() const;
    void allocate_value(Graph &graph, data::zone &zone);
    void destroy_value(Graph &graph);

    // TODO: check this method exists
    void destroy(Graph &graph);
};

} // namespace AG

CF_ASSUME_NONNULL_END
