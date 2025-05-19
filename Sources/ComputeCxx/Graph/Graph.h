#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Attribute/AttributeID.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType;

class Graph {
  public:
    class Context;

  private:
    util::Heap _heap;

    // Contexts
    util::Table<uint64_t, Context *> _contexts_by_id;

    // Threads
    uint32_t _ref_count = 1;

  public:
    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    Graph();

    // MARK: Context

    inline static void retain(Graph *graph) { graph->_ref_count += 1; };
    inline static void release(Graph *graph) {
        graph->_ref_count -= 1;
        if (graph->_ref_count == 0) {
            delete graph;
        }
    };

    // MARK: Attribute types

    const AttributeType &attribute_type(uint32_t type_id) const;
    const AttributeType &attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const;

    void did_allocate_node_value(size_t size);
    void did_destroy_node_value(size_t size);

    void update_attribute(AttributeID attribute, bool option);
};

} // namespace AG

CF_ASSUME_NONNULL_END
