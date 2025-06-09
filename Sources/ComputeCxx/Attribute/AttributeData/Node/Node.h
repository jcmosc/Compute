#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeData/Edge/InputEdge.h"
#include "Attribute/AttributeData/Edge/OutputEdge.h"
#include "Attribute/AttributeID/AGAttribute.h"
#include "Attribute/AttributeID/RelativeAttributeID.h"
#include "Data/Pointer.h"
#include "Data/Vector.h"

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
    unsigned int _dirty : 1;
    unsigned int _pending : 1;
    unsigned int _main_thread : 1;
    unsigned int _main_thread_only : 1;

    // Node state
    unsigned int _value_initialized : 1;
    unsigned int _self_initialized : 1;
    unsigned int _update_count : 2;
    unsigned int _type_id : 24;

    // Subgraph
    RelativeAttributeID _next_attribute;
    AGAttributeFlags _subgraph_flags;

    // Data flags
    unsigned int _has_indirect_self : 1;
    unsigned int _has_indirect_value : 1;
    unsigned int _input_edges_traverse_contexts : 1;
    unsigned int _input_edges_unsorted : 1;
    unsigned int _cacheable : 1;
    unsigned int _main_ref : 1;
    unsigned int _self_modified : 1;

    // Data
    data::ptr<void> _value;
    data::vector<InputEdge> _input_edges;
    data::vector<OutputEdge> _output_edges;

  public:
    Node(uint32_t type_id, bool main_thread)
        : _type_id(type_id), _main_thread(main_thread), _main_thread_only(main_thread) {};

    bool is_dirty() const { return _dirty; }
    void set_dirty(bool value) { _dirty = value; }

    bool is_pending() const { return _pending; }
    void set_pending(bool value) { _pending = value; }

    bool is_main_thread() const { return _main_thread; }
    bool is_main_thread_only() const { return _main_thread_only; }

    void set_value_initialized(bool value) { _value_initialized = value; }
    void set_self_initialized(bool value) { _self_initialized = value; }

    // TODO: test this
    uint8_t value_state() const {
        return _dirty << 0 | _pending << 1 | (_update_count > 0 ? 1 : 0) << 2 | _value_initialized << 3 |
               _main_thread << 4 | _main_ref << 5 | _main_thread_only << 6 | _self_modified << 7;
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

    bool is_main_ref() const { return _main_ref; }
    void set_main_ref(bool value) { _main_ref = value; }

    void sort_input_edges_if_needed() {
        if (_input_edges_unsorted) {
            _input_edges_unsorted = 0;
            std::sort(_input_edges.begin(), _input_edges.end());
        }
    }

    const data::vector<InputEdge> &input_edges() const { return _input_edges; };
    const data::vector<OutputEdge> &output_edges() const { return _output_edges; };

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
