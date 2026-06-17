#include "IAGGraph-Private.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include <Utilities/FreeDeleter.h>
#include <platform/lock.h>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Context.h"
#include "Graph.h"
#include "Trace/ExternalTrace.h"
#include "UpdateStack.h"

namespace {

CFRuntimeClass &graph_type_id() {
    static auto finalize = [](CFTypeRef graph_ref) {
        IAGGraphStorage *storage = (IAGGraphRef)graph_ref;
        if (!storage->context.invalidated()) {
            storage->context.IAG::Graph::Context::~Context();
        }
    };
    static CFRuntimeClass klass = {
        0,                // version
        "IAGGraphStorage", // className
        NULL,             // init
        NULL,             // copy
        finalize,
        NULL, // equal
        NULL, // hash
        NULL, // copyFormattingDesc
        NULL, // copyDebugDesc,
        NULL, // reclaim
        NULL, // refcount
        0     // requiredAlignment
    };
    return klass;
}

} // namespace

CFTypeID IAGGraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&graph_type_id());
    return type;
}

IAGGraphRef IAGGraphCreate() { return IAGGraphCreateShared(nullptr); };

IAGGraphRef IAGGraphCreateShared(IAGGraphRef original) {
    CFIndex extra_bytes = sizeof(struct IAGGraphStorage) - sizeof(CFRuntimeBase);
    IAGGraphRef instance =
        (IAGGraphRef)_CFRuntimeCreateInstance(kCFAllocatorDefault, IAGGraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        IAG::precondition_failure("memory allocation failure.");
    }

    IAG::Graph *graph;
    if (original) {
        if (original->context.invalidated()) {
            IAG::precondition_failure("invalidated graph");
        }
        graph = &original->context.graph();
        IAG::Graph::retain(graph);
    } else {
        graph = new IAG::Graph();
    }

    new (&instance->context) IAG::Graph::Context(graph); // performs a retain
    IAG::Graph::release(graph);

    instance->context.set_invalidated(false);

    return instance;
};

IAGUnownedGraphContextRef IAGGraphGetGraphContext(IAGGraphRef graph) {
    IAG::Graph::Context *graph_context = IAG::Graph::Context::from_cf(graph);
    IAG::Graph *unowned_graph = &graph_context->graph();
    return reinterpret_cast<IAGUnownedGraphContextRef>(unowned_graph);
}

IAGGraphRef IAGGraphContextGetGraph(void *storage) {
    auto graph_context = reinterpret_cast<IAG::Graph::Context *>(storage);
    return graph_context->to_cf();
}

void IAGGraphInvalidate(IAGGraphRef graph) {
    if (graph->context.invalidated()) {
        return;
    }
    graph->context.~Context();
    graph->context.set_invalidated(true);
}

#pragma mark - User context

const void *IAGGraphGetContext(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    return graph_context->context();
}

void IAGGraphSetContext(IAGGraphRef graph, const void *context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->set_context(context);
}

#pragma mark - Counter

uint64_t IAGGraphGetCounter(IAGGraphRef graph, IAGGraphCounterQueryType query) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    switch (query) {
    case IAGGraphCounterQueryTypeNodes:
        return graph_context->graph().num_nodes();
    case IAGGraphCounterQueryTypeTransactions:
        return graph_context->graph().transaction_count();
    case IAGGraphCounterQueryTypeUpdates:
        return graph_context->graph().update_count();
    case IAGGraphCounterQueryTypeChanges:
        return graph_context->graph().change_count();
    case IAGGraphCounterQueryTypeContextID:
        return graph_context->id();
    case IAGGraphCounterQueryTypeGraphID:
        return graph_context->graph().id();
    case IAGGraphCounterQueryTypeContextThreadUpdating:
        return graph_context->thread_is_updating();
    case IAGGraphCounterQueryTypeThreadUpdating:
        return graph_context->graph().thread_is_updating();
    case IAGGraphCounterQueryTypeContextNeedsUpdate:
        return graph_context->needs_update();
    case IAGGraphCounterQueryTypeNeedsUpdate:
        return graph_context->graph().needs_update();
    case IAGGraphCounterQueryTypeMainThreadUpdates:
        return graph_context->graph().main_thread_update_count();
    case IAGGraphCounterQueryTypeCreatedNodes:
        return graph_context->graph().num_nodes_total();
    case IAGGraphCounterQueryTypeSubgraphs:
        return graph_context->graph().num_subgraphs();
    case IAGGraphCounterQueryTypeCreatedSubgraphs:
        return graph_context->graph().num_subgraphs_total();
    default:
        return 0;
    }
}

#pragma mark - Main handler

void IAGGraphWithMainThreadHandler(IAGGraphRef graph,
                                  void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                  const void *body_context,
                                  void (*main_thread_handler)(void (*trampoline_thunk)(const void *),
                                                              const void *trampoline,
                                                              const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().with_main_handler(IAG::ClosureFunctionVV<void>(body, body_context), main_thread_handler,
                                             main_thread_handler_context);
}

#pragma mark - Subgraph

bool IAGGraphBeginDeferringSubgraphInvalidation(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    return graph_context->graph().begin_deferring_subgraph_invalidation();
}

void IAGGraphEndDeferringSubgraphInvalidation(IAGGraphRef graph, bool was_deferring) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().end_deferring_subgraph_invalidation(was_deferring);
}

#pragma mark - Attribute types

uint32_t IAGGraphInternAttributeType(IAGUnownedGraphContextRef unowned_graph, IAGTypeID type,
                                    const IAGAttributeType *(*make_attribute_type)(const void *context IAG_SWIFT_CONTEXT)
                                        IAG_SWIFT_CC(swift),
                                    const void *make_attribute_type_context) {
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    IAG::Graph *graph = reinterpret_cast<IAG::Graph *>(unowned_graph);
    return graph->intern_type(
        metadata, IAG::ClosureFunctionVP<const IAGAttributeType *>(make_attribute_type, make_attribute_type_context));
}

void IAGGraphVerifyType(IAGAttribute attribute, IAGTypeID type) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    if (auto node = attribute_id.get_node()) {
        auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
        auto attribute_type = subgraph->graph()->attribute_type(node->type_id());
        if (&attribute_type.value_metadata() != metadata) {
            IAG::precondition_failure("type check failed: %u, expected %s, got %s", attribute, metadata->name(false),
                                     attribute_type.value_metadata().name(false));
        }
    }
}

#pragma mark - Attributes

IAGAttribute IAGGraphCreateAttribute(uint32_t type_id, const void *body, const void *_Nullable value) {
    auto subgraph = IAG::Subgraph::current_subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no subgraph active while adding attribute");
    }
    auto node = subgraph->graph()->add_attribute(*subgraph, type_id, body, value);
    return IAGAttribute(IAG::AttributeID(node));
}

IAGGraphRef IAGGraphGetAttributeGraph(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    if (auto subgraph = attribute_id.subgraph()) {
        if (auto context_id = subgraph->context_id()) {
            if (auto context = subgraph->graph()->context_with_id(context_id)) {
                return context->to_cf();
            }
        }
    }
    IAG::precondition_failure("no graph: %u", attribute);
}

IAGAttributeInfo IAGGraphGetAttributeInfo(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }

    node.assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    const void *body = nullptr;
    const IAG::AttributeType &type = subgraph->graph()->attribute_ref(node, &body);
    return IAGAttributeInfo(reinterpret_cast<const IAGAttributeType *>(&type), body);
}

IAGAttributeFlags IAGGraphGetFlags(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }

    return node->subgraph_flags();
}

void IAGGraphSetFlags(IAGAttribute attribute, IAGAttributeFlags flags) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }

    attribute_id.subgraph()->set_flags(node, flags);
}

uint32_t IAGGraphAddInput(IAGAttribute attribute, IAGAttribute input, IAGInputOptions options) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    node.assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    auto input_attribute_id = IAG::AttributeID(input);
    input_attribute_id.validate_data_offset();

    if (input_attribute_id.subgraph() != nullptr && input_attribute_id.subgraph()->graph() != subgraph->graph()) {
        IAG::precondition_failure("accessing attribute in a different namespace: %u", input);
    }

    return subgraph->graph()->add_input(node, input_attribute_id, false, options);
}

#pragma mark - Offset attributes

namespace {

IAG::AttributeID create_offset_attribute(IAG::AttributeID attribute_id, uint32_t offset, std::optional<uint32_t> size) {
    if (offset == 0) {
        if (size.has_value() && attribute_id.is_indirect_node()) {
            auto calculated_size = attribute_id.size();
            if (calculated_size.has_value() && calculated_size.value() == size.value()) {
                return attribute_id;
            }
        }
    } else if (offset > IAG::IndirectNode::MaximumOffset) {
        IAG::precondition_failure("invalid offset: %u, %lu", offset, size.value());
    }

    auto current_subgraph = IAG::Subgraph::current_subgraph();
    if (!current_subgraph) {
        IAG::precondition_failure("no subgraph active while adding attribute");
    }

    IAG::data::ptr<IAG::IndirectNode> indirect_node =
        current_subgraph->graph()->add_indirect_attribute(*current_subgraph, attribute_id, offset, size, false);
    return IAG::AttributeID(indirect_node);
}

} // namespace

IAGAttribute IAGGraphCreateOffsetAttribute(IAGAttribute attribute, uint32_t offset) {
    auto attribute_id = IAG::AttributeID(attribute);
    return create_offset_attribute(attribute_id, offset, std::optional<size_t>());
}

IAGAttribute IAGGraphCreateOffsetAttribute2(IAGAttribute attribute, uint32_t offset, size_t size) {
    auto attribute_id = IAG::AttributeID(attribute);
    return create_offset_attribute(attribute_id, offset, std::optional<size_t>(size));
}

#pragma mark - Indirect attributes

namespace {

IAG::AttributeID create_indirect_attribute(IAG::AttributeID attribute_id, std::optional<uint32_t> size) {
    auto current_subgraph = IAG::Subgraph::current_subgraph();
    if (!current_subgraph) {
        IAG::precondition_failure("no subgraph active while making indirection");
    }

    IAG::data::ptr<IAG::IndirectNode> indirect_node =
        current_subgraph->graph()->add_indirect_attribute(*current_subgraph, attribute_id, 0, size, true);
    return IAG::AttributeID(indirect_node);
}

} // namespace

IAGAttribute IAGGraphCreateIndirectAttribute(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    return create_indirect_attribute(attribute_id, std::optional<size_t>());
}

IAGAttribute IAGGraphCreateIndirectAttribute2(IAGAttribute attribute, size_t size) {
    auto attribute_id = IAG::AttributeID(attribute);
    return create_indirect_attribute(attribute_id, std::optional<size_t>(size));
}

IAGAttribute IAGGraphGetIndirectAttribute(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    if (auto indirect_node = attribute_id.get_indirect_node()) {
        return indirect_node->source().identifier();
    }
    return attribute_id;
}

void IAGGraphSetIndirectAttribute(IAGAttribute attribute, IAGAttribute source) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto indirect_node = attribute_id.get_indirect_node();
    if (!indirect_node || !attribute_id.subgraph()) {
        IAG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    auto source_id = IAG::AttributeID(source);
    if (source_id.is_nil()) {
        attribute_id.subgraph()->graph()->indirect_attribute_reset(indirect_node, false);
    } else {
        attribute_id.subgraph()->graph()->indirect_attribute_set(indirect_node, source_id);
    }
}

void IAGGraphResetIndirectAttribute(IAGAttribute attribute, bool non_nil) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto indirect_node = attribute_id.get_indirect_node();
    if (!indirect_node || !attribute_id.subgraph()) {
        IAG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    attribute_id.subgraph()->graph()->indirect_attribute_reset(indirect_node, non_nil);
}

IAGAttribute IAGGraphGetIndirectDependency(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto indirect_node = attribute_id.get_indirect_node();
    if (!indirect_node || !attribute_id.subgraph()) {
        IAG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    return attribute_id.subgraph()->graph()->indirect_attribute_dependency(indirect_node);
}

void IAGGraphSetIndirectDependency(IAGAttribute attribute, IAGAttribute dependency) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto indirect_node = attribute_id.get_indirect_node();
    if (!indirect_node || !attribute_id.subgraph()) {
        IAG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    return attribute_id.subgraph()->graph()->indirect_attribute_set_dependency(indirect_node,
                                                                               IAG::AttributeID(dependency));
}

#pragma mark - Search

bool IAGGraphSearch(IAGAttribute attribute, IAGSearchOptions options,
                   bool (*predicate)(IAGAttribute attribute, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                   const void *predicate_context) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->breadth_first_search(
        attribute_id, options, IAG::ClosureFunctionAB<bool, IAGAttribute>(predicate, predicate_context));
}

#pragma mark - Body

void IAGGraphMutateAttribute(IAGAttribute attribute, IAGTypeID type, bool invalidating,
                            void (*modify)(void *body, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                            const void *modify_context) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->attribute_modify(node, *reinterpret_cast<const IAG::swift::metadata *>(type),
                                        IAG::ClosureFunctionPV<void, void *>(modify, modify_context), invalidating);
}

#pragma mark - Value

namespace {

inline IAGChangedValue get_value(IAG::AttributeID attribute_id, uint32_t seed, IAGValueOptions options,
                                const IAG::swift::metadata &metadata) {
    if (!(options & IAGValueOptionsIncrementGraphVersion)) {
        auto update_ptr = IAG::Graph::current_update();
        if (update_ptr.tag() == 0 && update_ptr.get() != nullptr) {
            auto &update = *update_ptr.get();

            auto graph = update.graph();
            auto &frame = update.frames().back();

            IAGChangedValueFlags flags = 0;
            void *value = graph->input_value_ref(frame.attribute, attribute_id, seed,
                                                 options & IAGValueOptionsInputOptionsMask, metadata, &flags);
            return {value, flags};
        }
    }

    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute_id);
    }

    IAGChangedValueFlags flags = 0;
    void *value = subgraph->graph()->value_ref(attribute_id, seed, metadata, &flags);
    return {value, flags};
}

} // namespace

IAGChangedValue IAGGraphGetValue(IAGAttribute attribute, IAGValueOptions options, IAGTypeID type) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    return get_value(attribute_id, 0, options, *metadata);
}

IAGWeakChangedValue IAGGraphGetWeakValue(IAGWeakAttribute attribute, IAGValueOptions options, IAGTypeID type) {
    auto weak_attribute_id = IAG::WeakAttributeID(attribute);
    auto attribute_id = weak_attribute_id.evaluate();
    if (attribute_id.is_nil()) {
        return {nullptr, false};
    }

    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    auto value = get_value(attribute_id, weak_attribute_id.seed(), options, *metadata);
    return *reinterpret_cast<IAGWeakChangedValue *>(&value);
}

IAGChangedValue IAGGraphGetInputValue(IAGAttribute attribute, IAGAttribute input, IAGValueOptions options, IAGTypeID type) {
    auto attribute_id = IAG::AttributeID(attribute);
    if (options & IAGValueOptionsIncrementGraphVersion || attribute_id.is_nil()) {
        return IAGGraphGetValue(input, options, type);
    }

    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    auto input_id = IAG::AttributeID(input);
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);

    IAGChangedValueFlags flags = 0;
    void *value = subgraph->graph()->input_value_ref(attribute, input_id, 0, options & IAGValueOptionsInputOptionsMask,
                                                     *metadata, &flags);
    return {value, flags};
}

bool IAGGraphSetValue(IAGAttribute attribute, const void *value, IAGTypeID type) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    return subgraph->graph()->value_set(attribute_id.get_node(), *metadata, value);
}

bool IAGGraphHasValue(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->value_exists(attribute_id.get_node());
}

IAGValueState IAGGraphGetValueState(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->value_state(attribute_id);
}

void IAGGraphUpdateValue(IAGAttribute attribute, IAGGraphUpdateOptions options) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }

    node.assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->update_attribute(node, options);
}

uint32_t IAGGraphPrefetchValue(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    if (subgraph->graph()->passed_deadline()) {
        return IAGGraphUpdateStatusChanged;
    }

    auto resolved = attribute_id.resolve(IAG::TraversalOptions::AssertNotNil);
    return (uint32_t)subgraph->graph()->update_attribute(
        resolved.attribute().get_node(),
        IAGGraphUpdateOptions(IAGGraphUpdateOptionsAbortIfCancelled | IAGGraphUpdateOptionsCancelIfPassedDeadline));
}

void IAGGraphInvalidateValue(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->value_mark(node);
}

void IAGGraphInvalidateAllValues(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().value_mark_all();
}

void IAGGraphSetInvalidationCallback(IAGGraphRef graph,
                                    void (*callback)(IAGAttribute, const void *context IAG_SWIFT_CONTEXT)
                                        IAG_SWIFT_CC(swift),
                                    const void *callback_context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->set_invalidation_callback(IAG::ClosureFunctionAV<void, IAGAttribute>(callback, callback_context));
}

#pragma mark - Cached value

namespace {

void *read_cached_attribute(size_t hash, const IAG::swift::metadata &metadata, const void *body,
                            const IAG::swift::metadata &value_metadata, IAGCachedValueOptions options,
                            IAG::AttributeID owner_id, IAGChangedValueFlags *flags_out,
                            IAG::ClosureFunctionCI<uint32_t, IAGUnownedGraphContextRef> get_attribute_type_id) {
    auto update = IAG::Graph::current_update();
    auto update_stack = update.tag() == 0 ? update.get() : nullptr;

    IAG::Subgraph *subgraph = nullptr;
    if (owner_id && !owner_id.is_nil()) {
        owner_id.validate_data_offset();
        subgraph = owner_id.subgraph();
    } else {
        if (update_stack != nullptr) {
            subgraph = IAG::AttributeID(update_stack->frames().back().attribute).subgraph();
        } else {
            subgraph = IAG::Subgraph::current_subgraph();
        }
    }
    if (subgraph == nullptr) {
        IAG::precondition_failure("no subgraph");
    }

    IAG::data::ptr<IAG::Node> cached_node = subgraph->cache_fetch(hash, metadata, body, get_attribute_type_id);
    if (cached_node == nullptr) {
        return nullptr;
    }

    if (update_stack == nullptr) {
        void *value = subgraph->graph()->value_ref(IAG::AttributeID(cached_node), 0, value_metadata, flags_out);
        subgraph->cache_insert(cached_node); // TODO: when this becomes an input, is it removed from cache?
        return value;
    }

    IAGInputOptions input_options = options & IAGCachedValueOptionsUnprefetched ? IAGInputOptionsUnprefetched : IAGInputOptionsNone;
    return subgraph->graph()->input_value_ref(update_stack->frames().back().attribute, IAG::AttributeID(cached_node), 0,
                                              input_options, value_metadata, flags_out);
}

} // namespace

void *IAGGraphReadCachedAttribute(size_t hash, IAGTypeID type, const void *body, IAGTypeID value_type,
                                 IAGCachedValueOptions options, IAGAttribute owner, bool *_Nullable changed_out,
                                 uint32_t (*closure)(IAGUnownedGraphContextRef graph_context,
                                                     const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                 const void *closure_context) {
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    auto value_metadata = reinterpret_cast<const IAG::swift::metadata *>(value_type);
    auto owner_id = IAG::AttributeID(owner);

    IAGChangedValueFlags flags = 0;
    void *value =
        read_cached_attribute(hash, *metadata, body, *value_metadata, options, owner_id, &flags,
                              IAG::ClosureFunctionCI<uint32_t, IAGUnownedGraphContextRef>(closure, closure_context));
    if (changed_out) {
        *changed_out = flags & IAGChangedValueFlagsChanged ? true : false;
    }
    return value;
}

void *IAGGraphReadCachedAttributeIfExists(size_t hash, IAGTypeID type, const void *body, IAGTypeID value_type,
                                         IAGCachedValueOptions options, IAGAttribute owner, bool *_Nullable changed_out) {

    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    auto value_metadata = reinterpret_cast<const IAG::swift::metadata *>(value_type);
    auto owner_id = IAG::AttributeID(owner);

    IAGChangedValueFlags flags = 0;
    void *value = read_cached_attribute(hash, *metadata, body, *value_metadata, options, owner_id, &flags, nullptr);
    if (changed_out) {
        *changed_out = flags & IAGChangedValueFlagsChanged ? true : false;
    }
    return value;
}

#pragma mark - Update

void IAGGraphSetUpdate(const void *update) {
    IAG::Graph::set_current_update(util::tagged_ptr<IAG::Graph::UpdateStack>((IAG::Graph::UpdateStack *)update));
}

const void *IAGGraphClearUpdate() {
    auto update = IAG::Graph::current_update();
    if (update != nullptr && update.tag() == 0) {
        IAG::Graph::set_current_update(update.with_tag(true));
    }
    return (const void *)update.value();
}

void IAGGraphCancelUpdate() {
    auto update = IAG::Graph::current_update().get();
    if (update == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    IAG::Graph::UpdateStack::cancel();
}

bool IAGGraphCancelUpdateIfNeeded() {
    auto update = IAG::Graph::current_update().get();
    if (update == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    if (update->cancelled()) {
        return true;
    }

    if (update->graph()->passed_deadline()) {
        update->cancel();
        return true;
    }

    return false;
}

bool IAGGraphUpdateWasCancelled() {
    auto update = IAG::Graph::current_update();
    if (update.tag() != 0 || update.get() == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    return update.get()->cancelled();
}

uint64_t IAGGraphGetDeadline(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    return graph_context->deadline();
}

void IAGGraphSetDeadline(IAGGraphRef graph, uint64_t deadline) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->set_deadline(deadline);
}

bool IAGGraphHasDeadlinePassed() {
    auto update = IAG::Graph::current_update().get();
    if (update != nullptr) {
        return update->graph()->passed_deadline();
    }
    return false;
}

void IAGGraphSetNeedsUpdate(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->set_needs_update();
}

void IAGGraphWithUpdate(IAGAttribute attribute, void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                       const void *body_context) {
    auto attribute_id = IAG::AttributeID(attribute);
    if (!attribute_id || attribute_id.is_nil()) {
        // TODO: check
        IAGGraphWithoutUpdate(body, body_context);
        return;
    }

    auto node = attribute_id.get_node();
    if (!node) {
        IAG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        IAG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->with_update(node, IAG::ClosureFunctionVV<void>(body, body_context));
}

void IAGGraphWithoutUpdate(void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                          const void *body_context) {
    IAG::Graph::without_update(IAG::ClosureFunctionVV<void>(body, body_context));
}

void IAGGraphSetUpdateCallback(IAGGraphRef graph,
                              void (*callback)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                              const void *callback_context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->set_update_callback(IAG::ClosureFunctionVV<void>(callback, callback_context));
}

IAGAttribute IAGGraphGetCurrentAttribute() {
    auto update_ptr = IAG::Graph::current_update();
    if (update_ptr.tag() != 0 || update_ptr.get() == nullptr) {
        return IAGAttributeNil;
    }

    auto &update = *update_ptr.get();
    if (update.frames().empty()) {
        return IAGAttributeNil;
    }
    auto &frame = update.frames().back();
    if (!frame.attribute) {
        return IAGAttributeNil;
    }

    return IAGAttribute(IAG::AttributeID(frame.attribute));
}

bool IAGGraphCurrentAttributeWasModified() {
    auto update_ptr = IAG::Graph::current_update();
    if (update_ptr.tag() != 0 || update_ptr.get() == nullptr) {
        return false;
    }

    auto &update = *update_ptr.get();
    auto &frame = update.frames().back();
    if (!frame.attribute) {
        return false;
    }

    return frame.attribute->is_self_modified();
}

bool IAGGraphAnyInputsChanged(const IAGAttribute *exclude_attributes, size_t exclude_attributes_count) {
    auto update = IAG::Graph::current_update();
    if (update.tag() != 0 || update.get() == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    auto &frame = update.get()->frames().back();
    return update.get()->graph()->any_inputs_changed(
        frame.attribute, reinterpret_cast<const IAG::AttributeID *>(exclude_attributes), exclude_attributes_count);
}

void *IAGGraphGetOutputValue(IAGTypeID type) {
    auto update_ptr = IAG::Graph::current_update();
    if (update_ptr.tag() != 0 || update_ptr.get() == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    auto &update = *update_ptr.get();
    auto &frame = update.frames().back();
    auto graph = update.graph();
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    return graph->output_value_ref(frame.attribute, *metadata);
}

void IAGGraphSetOutputValue(const void *value, IAGTypeID type) {
    auto update_ptr = IAG::Graph::current_update();
    if (update_ptr.tag() != 0 || update_ptr.get() == nullptr) {
        IAG::precondition_failure("no attribute updating");
    }

    auto &update = *update_ptr.get();
    auto &frame = update.frames().back();
    if (!frame.attribute->is_updating()) {
        IAG::precondition_failure("writing attribute that is not evaluating: %", frame.attribute);
    }

    auto graph = update.graph();
    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    graph->value_set_internal(frame.attribute, *frame.attribute.get(), value, *metadata);
}

#pragma mark - Trace

void IAGGraphStartTracing(IAGGraphRef graph, IAGGraphTraceOptions trace_options) { IAGGraphStartTracing2(graph, trace_options, NULL); }

void IAGGraphStartTracing2(IAGGraphRef graph, IAGGraphTraceOptions trace_options, CFArrayRef subsystems) {
    auto subsystems_vector = IAG::vector<std::unique_ptr<const char, util::free_deleter>, 0, uint64_t>();
    if (subsystems) {
        auto subsystems_count = CFArrayGetCount(subsystems);
        for (CFIndex index = 0; index < subsystems_count; ++index) {
            CFTypeRef value = CFArrayGetValueAtIndex(subsystems, index);
            if (CFGetTypeID(value) != CFStringGetTypeID()) {
                continue;
            }

            CFIndex length = CFStringGetLength((CFStringRef)value);
            CFIndex bufferSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
            char *subsystem = (char *)malloc(bufferSize);
            if (CFStringGetCString((CFStringRef)value, subsystem, bufferSize, kCFStringEncodingUTF8)) {
                subsystems_vector.push_back(std::unique_ptr<const char, util::free_deleter>(subsystem));
            } else {
                free(subsystem);
            }
        }
    }

    std::span<const char *> subsystems_span =
        std::span<const char *>((const char **)subsystems_vector.data(), subsystems_vector.size());

    if (graph == nullptr) {
        IAG::Graph::all_start_tracing(trace_options, subsystems_span);
        return;
    }

    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().start_tracing(trace_options, subsystems_span);
}

void IAGGraphStopTracing(IAGGraphRef graph) {
    if (graph == nullptr) {
        IAG::Graph::all_stop_tracing();
        return;
    }

    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().stop_tracing();
}

void IAGGraphSyncTracing(IAGGraphRef graph) {
    if (graph == nullptr) {
        IAG::Graph::all_sync_tracing();
        return;
    }

    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().sync_tracing();
}

CFStringRef IAGGraphCopyTracePath(IAGGraphRef graph) {
    if (graph == nullptr) {
        return IAG::Graph::all_copy_trace_path();
    }

    auto graph_context = IAG::Graph::Context::from_cf(graph);
    return graph_context->graph().copy_trace_path();
}

uint64_t IAGGraphAddTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    auto external_trace = new ExternalTrace(trace, context);
    graph_context->graph().add_trace(external_trace);
    return external_trace->id();
}

void IAGGraphRemoveTrace(IAGGraphRef graph, uint64_t trace_id) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().remove_trace(trace_id);
}

void IAGGraphSetTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().remove_trace(0);

    auto external_trace = new ExternalTrace(0, trace, context);
    graph_context->graph().add_trace(external_trace);
}

void IAGGraphResetTrace(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().remove_trace(0);
}

bool IAGGraphIsTracingActive(IAGGraphRef graph) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    return graph_context->graph().traces().size() > 0;
}

void IAGGraphPrepareTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *context) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    auto external_trace = new ExternalTrace(trace, context);
    graph_context->graph().prepare_trace(*external_trace);
}

bool IAGGraphTraceEventEnabled(IAGGraphRef graph, uint32_t event_id) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    for (auto trace : graph_context->graph().traces()) {
        if (trace->named_event_enabled(event_id)) {
            return true;
        }
    }
    return false;
}

void IAGGraphAddTraceEvent(IAGGraphRef graph, const char *event_name, const void *value, IAGTypeID type) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().foreach_trace([&graph_context, &event_name, &value, &type](IAG::Trace &trace) {
        trace.custom_event(*graph_context, event_name, value, *reinterpret_cast<const IAG::swift::metadata *>(type));
    });
}

void IAGGraphAddNamedTraceEvent(IAGGraphRef graph, uint32_t event_id, uint32_t event_arg_count, const void *event_args,
                               CFDataRef data, uint32_t arg6) {
    auto graph_context = IAG::Graph::Context::from_cf(graph);
    graph_context->graph().foreach_trace(
        [&graph_context, &event_id, &event_arg_count, &event_args, &data, &arg6](IAG::Trace &trace) {
            trace.named_event(*graph_context, event_id, event_arg_count, event_args, data, arg6);
        });
}

namespace NamedEvents {

static platform_lock lock = PLATFORM_LOCK_INIT;
static IAG::vector<std::pair<const char *, const char *>, 0, uint32_t> *names;

} // namespace NamedEvents

const char *IAGGraphGetTraceEventName(uint32_t event_id) {
    const char *event_name = nullptr;

    platform_lock_lock(&NamedEvents::lock);
    if (NamedEvents::names != nullptr && event_id < NamedEvents::names->size()) {
        event_name = (*NamedEvents::names)[event_id].second;
    }
    platform_lock_unlock(&NamedEvents::lock);

    return event_name;
}

const char *IAGGraphGetTraceEventSubsystem(uint32_t event_id) {
    const char *event_subsystem = nullptr;

    platform_lock_lock(&NamedEvents::lock);
    if (NamedEvents::names != nullptr && event_id < NamedEvents::names->size()) {
        event_subsystem = (*NamedEvents::names)[event_id].first;
    }
    platform_lock_unlock(&NamedEvents::lock);

    return event_subsystem;
}

uint32_t IAGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem) {
    platform_lock_lock(&NamedEvents::lock);

    if (!NamedEvents::names) {
        NamedEvents::names = new IAG::vector<std::pair<const char *, const char *>, 0, uint32_t>();
        NamedEvents::names->push_back({0, 0}); // Disallow 0 as event ID
    }

    uint32_t event_id = NamedEvents::names->size();
    if (event_subsystem != nullptr) {
        event_subsystem = strdup(event_subsystem);
    }
    event_name = strdup(event_name);
    NamedEvents::names->push_back({event_subsystem, event_name});

    platform_lock_unlock(&NamedEvents::lock);

    return event_id;
}

// MARK: Description

#if TARGET_OS_MAC
void IAGGraphArchiveJSON(const char *filename) { IAG::Graph::write_to_file(nullptr, filename, false); }

void IAGGraphArchiveJSON2(const char *filename, bool exclude_values) {
    IAG::Graph::write_to_file(nullptr, filename, exclude_values);
}
#endif
