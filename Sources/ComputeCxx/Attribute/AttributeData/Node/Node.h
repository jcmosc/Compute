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

class Node {
  private:
    // Value state
    unsigned int _dirty : 1 = 0;
    unsigned int _pending : 1 = 0;
    unsigned int _main_thread : 1 = 0;
    unsigned int _main_thread_only : 1 = 0;

    // Node state
    unsigned int _value_initialized : 1 = 0;
    unsigned int _self_initialized : 1 = 0;
    unsigned int _updating : 1 = 0;
    unsigned int _updating_cyclic : 1 = 0;
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
        : _type_id(type_id), _main_thread(main_thread), _main_thread_only(main_thread) {};

    // Non-copyable
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    // Non-movable
    Node(Node &&) = delete;
    Node &operator=(Node &&) = delete;

    bool is_dirty() const { return _dirty; }
    void set_dirty(bool value) { _dirty = value; }

    bool is_pending() const { return _pending; }
    void set_pending(bool value) { _pending = value; }

    bool is_main_thread() const { return _main_thread; }

    bool is_main_thread_only() const { return _main_thread_only; }
    void set_main_thread_only(bool value) { _main_thread_only = value; }

    bool is_value_initialized() const { return _value_initialized; }
    void set_value_initialized(bool value) { _value_initialized = value; }

    bool is_self_initialized() const { return _self_initialized; }
    void set_self_initialized(bool value) { _self_initialized = value; }

    bool is_updating() const { return _updating || _updating_cyclic; };
    bool set_updating(bool value) { _updating = value; };

    // TODO: test this
    AGValueState flags() const {
        return (_dirty ? AGValueStateDirty : 0) | (_pending ? AGValueStatePending : 0) |
               (_updating || _updating_cyclic ? AGValueStateUpdating : 0) |
               (_value_initialized ? AGValueStateValueExists : 0) | (_main_thread ? AGValueStateMainThread : 0) |
               (_main_ref ? AGValueStateMainRef : 0) | (_main_thread_only ? AGValueStateMainThreadOnly : 0) |
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

    void set_input_edges_traverse_contexts(bool value) { _input_edges_traverse_contexts = value; }

    bool needs_sort_input_edges() { return _needs_sort_input_edges; }
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

    data::vector<InputEdge> &input_edges() { return _input_edges; }; // TODO: delete whole vector accessor
    void add_input_edge(data::zone *subgraph, InputEdge &input_edge) {
        _input_edges.push_back(subgraph, input_edge);
        _needs_sort_input_edges = true;
    }

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
