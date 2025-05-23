#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <memory>
#include <stdint.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "Closure/ClosureFunction.h"
#include "Swift/Metadata.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph {
  public:
    class Context;

  private:
    util::Heap _heap;

    // Attribute types
    util::UntypedTable _interned_types;
    vector<std::unique_ptr<AttributeType, AttributeType::deleter>, 0, uint32_t> _types;

    // Contexts
    util::Table<uint64_t, Context *> _contexts_by_id;

    // Threads
    uint32_t _ref_count = 1;

    uint64_t _id;

  public:
    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);

    Graph();

    uint64_t id() const { return _id; }

    // MARK: Context

    inline static void retain(Graph *graph) { graph->_ref_count += 1; };
    inline static void release(Graph *graph) {
        graph->_ref_count -= 1;
        if (graph->_ref_count == 0) {
            delete graph;
        }
    };

    // MARK: Attribute types

    const AttributeType &attribute_type(uint32_t type_id) const { return *_types[type_id]; };
    const AttributeType &attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const;

    uint32_t intern_type(const swift::metadata *metadata, ClosureFunctionVP<const AGAttributeType *> make_type);

    void did_allocate_node_value(size_t size);
    void did_destroy_node_value(size_t size);

    void update_attribute(AttributeID attribute, bool option);

    // MARK: Description

#ifdef __OBJC__
    static NSObject *_Nullable description(Graph *_Nullable graph, NSDictionary *options);
    static NSDictionary *description_graph(Graph *_Nullable graph, NSDictionary *options);
#endif
};

} // namespace AG

CF_ASSUME_NONNULL_END
