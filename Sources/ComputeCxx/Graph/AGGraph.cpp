#include "AGGraph-Private.h"

#include "Attribute/AttributeID.h"
#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/OffsetAttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Context.h"
#include "External/ExternalTrace.h"
#include "Graph.h"
#include "Private/CFRuntime.h"
#include "Trace/Trace.h"
#include "UpdateStack.h"

namespace {

CFRuntimeClass &graph_type_id() {
    static auto finalize = [](CFTypeRef graph_ref) {
        AGGraphStorage *storage = (AGGraphRef)graph_ref;
        if (!storage->context.invalidated()) {
            storage->context.AG::Graph::Context::~Context();
        }
    };
    static CFRuntimeClass klass = {
        0,         // version
        "AGGraph", // className
        NULL,      // init
        NULL,      // copy,
        finalize,
        NULL, // equal
        NULL, // hash
        NULL, // copyFormattingDesc
        NULL, // copyDebugDesc,
        0     // ??
    };
    return klass;
}

} // namespace

CFTypeID AGGraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&graph_type_id());
    return type;
}

AGGraphRef AGGraphCreate() { return AGGraphCreateShared(nullptr); };

AGGraphRef AGGraphCreateShared(AGGraphRef original) {
    CFIndex extra_bytes = sizeof(struct AGGraphStorage) - sizeof(CFRuntimeBase);
    struct AGGraphStorage *instance =
        (struct AGGraphStorage *)_CFRuntimeCreateInstance(kCFAllocatorDefault, AGGraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        AG::precondition_failure("memory allocation failure.");
    }

    AG::Graph *graph;
    if (original) {
        if (original->context.invalidated()) {
            AG::precondition_failure("invalidated graph");
        }
        graph = &original->context.graph();
        AG::Graph::will_add_to_context(graph);
    } else {
        graph = new AG::Graph();
    }

    new (&instance->context) AG::Graph::Context(graph);

    AG::Graph::did_remove_from_context(graph);

    instance->context.set_invalidated(false);

    return instance;
};

AGUnownedGraphContextRef AGGraphGetGraphContext(AGGraphRef graph) {
    return reinterpret_cast<AGUnownedGraphContextRef>(AG::Graph::Context::from_cf(graph));
}

AGGraphRef AGGraphContextGetGraph(AGUnownedGraphContextRef context) {
    return reinterpret_cast<AGGraphRef>(reinterpret_cast<uintptr_t>(context) - sizeof(CFRuntimeBase));
}

const void *AGGraphGetContext(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    return context->context_info();
}

void AGGraphSetContext(AGGraphRef graph, const void *context_info) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->set_context_info(context_info);
}

#pragma mark - Counters

uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) {
    auto context = AG::Graph::Context::from_cf(graph);
    switch (query) {
    case AGGraphCounterQueryNodeCount:
        return context->graph().num_nodes();
    case AGGraphCounterQueryTransactionCount:
        return context->graph().transaction_count();
    case AGGraphCounterQueryUpdateCount:
        return context->graph().update_count();
    case AGGraphCounterQueryChangeCount:
        return context->graph().change_count();
    case AGGraphCounterQueryContextID:
        return context->unique_id();
    case AGGraphCounterQueryGraphID:
        return context->graph().unique_id();
    case AGGraphCounterQueryContextThreadUpdating:
        return context->thread_is_updating();
    case AGGraphCounterQueryThreadUpdating:
        return context->graph().thread_is_updating();
    case AGGraphCounterQueryContextNeedsUpdate:
        return context->needs_update();
    case AGGraphCounterQueryNeedsUpdate:
        return context->graph().needs_update();
    case AGGraphCounterQueryMainThreadUpdateCount:
        return context->graph().update_on_main_count();
    case AGGraphCounterQueryNodeTotalCount:
        return context->graph().num_nodes_created();
    case AGGraphCounterQuerySubgraphCount:
        return context->graph().num_subgraphs();
    case AGGraphCounterQuerySubgraphTotalCount:
        return context->graph().num_subgraphs_created();
    default:
        return 0;
    }
}

#pragma mark - Subgraph

bool AGGraphBeginDeferringSubgraphInvalidation(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    bool old_value = context->graph().is_deferring_subgraph_invalidation();
    context->graph().set_deferring_subgraph_invalidation(true);
    return old_value;
}

void AGGraphEndDeferringSubgraphInvalidation(AGGraphRef graph, bool was_deferring_subgraph_invalidation) {
    auto context = AG::Graph::Context::from_cf(graph);
    if (!was_deferring_subgraph_invalidation) {
        context->graph().set_deferring_subgraph_invalidation(false);
        context->graph().invalidate_subgraphs();
    }
}

#pragma mark - Attribute types

// TODO: is this AGGraphRef or AG::Graph ?
uint32_t AGGraphInternAttributeType(AGGraphRef graph, AGTypeID type,
                                    void *(*intern)(const void *context AG_SWIFT_CONTEXT)AG_SWIFT_CC(swift),
                                    const void *context) {
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    return graph->context.graph().intern_type(metadata, AG::ClosureFunctionVP<void *>(intern, context));
}

void AGGraphVerifyType(AGAttribute attribute, AGTypeID type) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    if (attribute_id.is_direct()) {
        auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
        auto attribute_type = subgraph->graph()->attribute_type(attribute_id.to_node().type_id());
        if (&attribute_type.value_metadata() != metadata) {
            AG::precondition_failure("type check failed: %u, expected %s, got %s", attribute, metadata->name(false),
                                     attribute_type.value_metadata().name(false));
        }
    }
}

#pragma mark - Attributes

AGAttribute AGGraphCreateAttribute(uint32_t type_id, void *body, void *value) {
    auto current_subgraph = AG::Subgraph::current_subgraph();
    if (!current_subgraph) {
        AG::precondition_failure("no subgraph active while adding attribute");
    }
    return current_subgraph->graph()->add_attribute(*current_subgraph, type_id, body, value);
}

AGGraphRef AGGraphGetAttributeGraph(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    if (auto subgraph = attribute_id.subgraph()) {
        auto context_id = subgraph->context_id();
        if (context_id != 0) {
            if (auto context = subgraph->graph()->context_with_id(context_id)) {
                return context->to_cf();
            }
        }
    }
    AG::precondition_failure("no graph: %u", attribute);
}

// AGSubgraphRef AGGraphGetAttributeSubgraph(AGAttribute attribute) {
//     auto subgraph = AGGraphGetAttributeSubgraph2(attribute);
//     if (subgraph == nullptr) {
//         AG::precondition_failure("no subgraph");
//     }
//
//     return subgraph;
// }
//
// AGSubgraphRef AGGraphGetAttributeSubgraph2(AGAttribute attribute) {
//     auto attribute_id = AG::AttributeID(attribute);
//     attribute_id.to_node_ptr().assert_valid();
//
//     auto subgraph = attribute_id.subgraph();
//     if (subgraph == nullptr) {
//         AG::precondition_failure("internal error");
//     }
//
//     return subgraph->to_cf();
// }

AGAttributeInfo AGGraphGetAttributeInfo(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    const void *body = nullptr;
    auto type = subgraph->graph()->attribute_ref(attribute_id.to_node_ptr(), &body);
    return AGAttributeInfo(type, body);
}

uint8_t AGGraphGetFlags(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    return attribute_id.to_node().flags().subgraph_flags();
}

void AGGraphSetFlags(AGAttribute attribute, uint8_t flags) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.subgraph()->set_flags(attribute_id.to_node_ptr(), AG::NodeFlags::SubgraphFlags(flags));
}

void AGGraphMutateAttribute(AGAttribute attribute, AGTypeID type, bool flag,
                            void (*modify)(void *body, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                            const void *context) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->attribute_modify(attribute_id.to_node_ptr(),
                                        *reinterpret_cast<const AG::swift::metadata *>(type),
                                        AG::ClosureFunctionPV<void, void *>(modify, context), flag);
}

bool AGGraphSearch(AGAttribute attribute, AGSearchOptions options,
                   bool (*predicate)(uint32_t attribute, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                   const void *context) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->breadth_first_search(attribute_id, AG::Graph::SearchOptions(options),
                                                   AG::ClosureFunctionAB<bool, uint32_t>(predicate, context));
}

#pragma mark - Cached attributes

namespace {

void read_cached_attribute(unsigned long, AGSwiftMetadata const *, void const *, AGSwiftMetadata const *, unsigned int,
                           unsigned int, unsigned char &,
                           AG::ClosureFunctionCI<unsigned long, AGUnownedGraphContext *>) {}

} // namespace

void AGGraphReadCachedAttribute() {
    // TODO: not implemented
}

void AGGraphReadCachedAttributeIfExists() {
    // TODO: not implemented
}

#pragma mark - Current attribute

AGAttribute AGGraphGetCurrentAttribute() {
    auto update = AG::Graph::current_update();
    if (update.tag() == 0) {
        if (auto update_stack = update.get()) {
            auto frame = update_stack->frames().back();
            if (frame.attribute) {
                return frame.attribute;
            }
        }
    }
    return AGAttributeNil;
}

bool AGGraphCurrentAttributeWasModified() {
    auto update = AG::Graph::current_update();
    if (update.tag() == 0) {
        if (auto update_stack = update.get()) {
            auto frame = update_stack->frames().back();
            if (frame.attribute) {
                return frame.attribute->flags().self_modified();
            }
        }
    }
    return false;
}

#pragma mark - Indirect attributes

namespace {

AG::AttributeID create_indirect_attribute(AG::AttributeID attribute_id, std::optional<uint32_t> size) {
    auto current_subgraph = AG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        AG::precondition_failure("no subgraph active while making indirection");
    }

    AG::data::ptr<AG::IndirectNode> indirect_node =
        current_subgraph->graph()->add_indirect_attribute(*current_subgraph, attribute_id, 0, size, true);
    return AG::AttributeID(indirect_node); // TODO: check adds kind
}

} // namespace

AGAttribute AGGraphCreateIndirectAttribute(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    return create_indirect_attribute(attribute_id, std::optional<size_t>());
}

AGAttribute AGGraphCreateIndirectAttribute2(AGAttribute attribute, size_t size) {
    auto attribute_id = AG::AttributeID(attribute);
    return create_indirect_attribute(attribute_id, std::optional<size_t>(size));
}

AGAttribute AGGraphGetIndirectAttribute(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_indirect()) {
        return attribute_id;
    }
    return attribute_id.to_indirect_node().source().attribute();
}

void AGGraphSetIndirectAttribute(AGAttribute attribute, AGAttribute source) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_indirect() || attribute_id.subgraph() == nullptr) {
        AG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    auto source_id = AG::AttributeID(source);
    if (source_id.is_nil()) {
        attribute_id.subgraph()->graph()->indirect_attribute_reset(attribute_id.to_indirect_node_ptr(), false);
    } else {
        attribute_id.subgraph()->graph()->indirect_attribute_set(attribute_id.to_indirect_node_ptr(), source_id);
    }
}

void AGGraphResetIndirectAttribute(AGAttribute attribute, bool non_nil) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_indirect() || attribute_id.subgraph() == nullptr) {
        AG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    attribute_id.subgraph()->graph()->indirect_attribute_reset(attribute_id.to_indirect_node_ptr(), non_nil);
}

AGAttribute AGGraphGetIndirectDependency(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_indirect() || attribute_id.subgraph() == nullptr) {
        AG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    return attribute_id.subgraph()->graph()->indirect_attribute_dependency(attribute_id.to_indirect_node_ptr());
}

void AGGraphSetIndirectDependency(AGAttribute attribute, AGAttribute dependency) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_indirect() || attribute_id.subgraph() == nullptr) {
        AG::precondition_failure("invalid indirect attribute: %u", attribute);
    }

    return attribute_id.subgraph()->graph()->indirect_attribute_set_dependency(attribute_id.to_indirect_node_ptr(),
                                                                               AG::AttributeID(dependency));
}

void AGGraphRegisterDependency(AGAttribute dependency, uint8_t input_edge_flags) {
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.tag() != 0 || update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    auto update_stack = update_stack_ptr.get();
    auto graph = update_stack->graph();
    auto frame = update_stack->frames().back();

    graph->input_value_add(frame.attribute, AG::AttributeID(dependency), input_edge_flags);
}

#pragma mark - Offset attributes

namespace {

AG::AttributeID create_offset_attribute(AG::AttributeID attribute_id, uint32_t offset, std::optional<uint32_t> size) {
    if (offset == 0) {
        if (size.has_value() && attribute_id.is_indirect()) {
            auto calculated_size = attribute_id.size();
            if (calculated_size.has_value() && calculated_size.value() == size.value()) {
                return attribute_id;
            }
        }
    } else if (offset > AG::IndirectNode::MaximumOffset) {
        AG::precondition_failure("invalid offset: %u, %lu", offset, size.value());
    }

    auto current_subgraph = AG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        AG::precondition_failure("no subgraph active while adding attribute");
    }

    AG::data::ptr<AG::IndirectNode> indirect_node =
        current_subgraph->graph()->add_indirect_attribute(*current_subgraph, attribute_id, offset, size, false);
    return AG::AttributeID(indirect_node); // TODO: check adds kind
}

} // namespace

AGAttribute AGGraphCreateOffsetAttribute(AGAttribute attribute, uint32_t offset) {
    auto attribute_id = AG::AttributeID(attribute);
    return create_offset_attribute(attribute_id, offset, std::optional<size_t>());
}

AGAttribute AGGraphCreateOffsetAttribute2(AGAttribute attribute, uint32_t offset, size_t size) {
    auto attribute_id = AG::AttributeID(attribute);
    return create_offset_attribute(attribute_id, offset, std::optional<size_t>(size));
}

#pragma mark - Updates

void AGGraphInvalidate(AGGraphRef graph) {
    if (graph->context.invalidated()) {
        return;
    }
    graph->context.~Context(); // TODO: check this goes here
    graph->context.set_invalidated(true);
}

void AGGraphInvalidateAllValues(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().value_mark_all();
}

void AGGraphInvalidateValue(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->value_mark(attribute_id.to_node_ptr());
}

void AGGraphSetInvalidationCallback(AGGraphRef graph,
                                    void (*callback)(AGAttribute, const void *context AG_SWIFT_CONTEXT)
                                        AG_SWIFT_CC(swift),
                                    const void *callback_context) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->set_invalidation_callback(AG::ClosureFunctionAV<void, AGAttribute>(callback, callback_context));
}

void AGGraphUpdateValue(AGAttribute attribute, uint8_t options) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->update_attribute(attribute_id, options);
}

void AGGraphCancelUpdate() {
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    update_stack_ptr.get()->cancel();
}

bool AGGraphCancelUpdateIfNeeded() {
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    if (update_stack_ptr.get()->cancelled()) {
        return true;
    }

    if (update_stack_ptr.get()->graph()->passed_deadline()) {
        update_stack_ptr.get()->cancel();
        return true;
    }

    return false;
}

bool AGGraphUpdateWasCancelled() {
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.tag() != 0 || update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    return update_stack_ptr.get()->cancelled();
}

void AGGraphSetUpdate(const void *update) {
    AG::Graph::set_current_update(util::tagged_ptr<AG::Graph::UpdateStack>((AG::Graph::UpdateStack *)update));
}

void AGGraphSetUpdateCallback(AGGraphRef graph,
                              void (*callback)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                              const void *callback_context) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->set_update_callback(AG::ClosureFunctionVV<void>(callback, callback_context));
}

void AGGraphClearUpdate() {
    auto current_update = AG::Graph::current_update();
    if (current_update != nullptr && current_update.tag() == 0) {
        AG::Graph::set_current_update(current_update.with_tag(true)); // TODO: tag is cleared...
    }
}

uint64_t AGGraphGetDeadline(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    return context->deadline();
}

void AGGraphSetDeadline(AGGraphRef graph, uint64_t deadline) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->set_deadline(deadline);
}

bool AGGraphHasDeadlinePassed() {
    auto current_update = AG::Graph::current_update();
    if (current_update != nullptr) {
        return current_update.get()->graph()->passed_deadline();
    }
    return false;
}

void AGGraphSetNeedsUpdate(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->set_needs_update();
}

void AGGraphWithUpdate(AGAttribute attribute, void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                       const void *context) {
    auto attribute_id = AG::AttributeID(attribute);
    if (attribute_id.is_nil()) {
        // TODO: AGGraphWithoutUpdate
        auto update = AG::Graph::current_update();
        AG::Graph::set_current_update(update != nullptr ? update.with_tag(true) : nullptr);
        function(context);
        AG::Graph::set_current_update(update);
        return;
    }

    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    subgraph->graph()->with_update(attribute_id.to_node_ptr(), AG::ClosureFunctionVV<void>(function, context));
}

void AGGraphWithoutUpdate(void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                          const void *context) {
    AG::Graph::without_update(AG::ClosureFunctionVV<void>(function, context));
}

void AGGraphWithMainThreadHandler(AGGraphRef graph,
                                  void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *body_context,
                                  void (*main_thread_handler)(void (*thunk)(void *),
                                                              const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().with_main_handler(AG::ClosureFunctionVV<void>(function, body_context), main_thread_handler,
                                       main_thread_handler_context);
}

#pragma mark - Values

namespace {

// TODO: inline
AGValue get_value(AG::AttributeID attribute_id, uint32_t zone_id, AGValueOptions options,
                  const AG::swift::metadata &metadata) {
    if (!(options & AGValueOptions0x04)) {
        auto update_stack_ptr = AG::Graph::current_update();
        if (update_stack_ptr.tag() == 0 && update_stack_ptr.get() != nullptr) {
            auto update_stack = update_stack_ptr.get();

            auto graph = update_stack->graph();
            auto frame = update_stack->frames().back();

            uint8_t state = 0;
            void *value = graph->input_value_ref(frame.attribute, attribute_id, zone_id, options & 3, metadata, &state);

            // TODO: check if this is state or changed
            return {value, (state & 1) == 1};
        }
    }

    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute_id);
    }

    bool changed = false;
    void *value = subgraph->graph()->value_ref(attribute_id, zone_id, metadata, &changed);

    return {value, changed};
}

} // namespace

bool AGGraphHasValue(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->value_exists(attribute_id.to_node_ptr());
}

AGValue AGGraphGetValue(AGAttribute attribute, AGValueOptions options, AGTypeID type) {
    auto attribute_id = AG::AttributeID(attribute);
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    return get_value(attribute_id, 0, options, *metadata);
}

AGValue AGGraphGetWeakValue(AGWeakAttribute attribute, AGValueOptions options, AGTypeID type) {
    auto weak_attribute_id = AG::WeakAttributeID(attribute);
    auto attribute_id = weak_attribute_id.evaluate();
    if (attribute_id.is_nil()) {
        return {nullptr, false};
    }

    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    return get_value(attribute_id, weak_attribute_id.zone_id(), options, *metadata);
}

bool AGGraphSetValue(AGAttribute attribute, void *value, AGTypeID type) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    return subgraph->graph()->value_set(attribute_id.to_node_ptr(), *metadata, value);
}

AGGraphUpdateStatus AGGraphPrefetchValue(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr()
        .assert_valid(); // TODO: make assert_value on AttributeID, don't care about kind at this point

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    if (subgraph->graph()->passed_deadline()) {
        return AGGraphUpdateStatusChanged;
    }

    auto resolved = attribute_id.resolve(AG::TraversalOptions::AssertNotNil);

    // TODO: typed options
    return subgraph->graph()->update_attribute(resolved.attribute(), 6);
}

#pragma mark - Inputs

AGValue AGGraphGetInputValue(AGAttribute attribute, AGAttribute input_attribute, AGValueOptions options,
                             AGTypeID type) {
    auto attribute_id = AG::AttributeID(attribute);
    if (options & AGValueOptions0x04 || attribute_id.is_nil()) {
        return AGGraphGetValue(input_attribute, options, type);
    }

    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    auto input_attribute_id = AG::AttributeID(input_attribute);
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);

    uint8_t state = 0;
    void *value = subgraph->graph()->input_value_ref(attribute, input_attribute_id, 0, options & 3, *metadata, &state);

    // TODO: check if this is state or changed
    return {value, (state & 1) == 1};
}

uint32_t AGGraphAddInput(AGAttribute attribute, AGAttribute input, uint8_t input_edge_flags) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id.is_direct()) {
        AG::precondition_failure("non-direct attribute id: %u", attribute);
    }
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    auto input_attribute_id = AG::AttributeID(input);
    input_attribute_id.to_node_ptr().assert_valid();

    if (input_attribute_id.subgraph() != nullptr && input_attribute_id.subgraph()->graph() != subgraph->graph()) {
        AG::precondition_failure("accessing attribute in a different namespace: %u", input);
    }

    return subgraph->graph()->add_input(attribute_id.to_node_ptr(), input_attribute_id, false, input_edge_flags);
}

bool AGGraphAnyInputsChanged(const AGAttribute *exclude_attributes, uint64_t exclude_attributes_count) {
    // TODO: tag must be whether frames is empty or not
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.tag() != 0 || update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    auto update_stack = update_stack_ptr.get();
    auto frame = update_stack->frames().back();

    return update_stack->graph()->any_inputs_changed(
        frame.attribute, reinterpret_cast<const AG::AttributeID *>(exclude_attributes), exclude_attributes_count);
}

#pragma mark - Outputs

void *AGGraphGetOutputValue(AGTypeID type) {
    // TODO: tag must be whether frames is empty or not
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.tag() != 0 || update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    auto update_stack = update_stack_ptr.get();
    auto frame = update_stack->frames().back();

    auto graph = update_stack->graph();
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    return graph->output_value_ref(frame.attribute, *metadata);
}

void AGGraphSetOutputValue(void *value, AGTypeID type) {
    // TODO: tag must be whether frames is empty or not
    auto update_stack_ptr = AG::Graph::current_update();
    if (update_stack_ptr.tag() != 0 || update_stack_ptr.get() == nullptr) {
        AG::precondition_failure("no attribute updating");
    }

    auto update_stack = update_stack_ptr.get();
    auto frame = update_stack->frames().back();

    if (!frame.attribute->state().is_updating()) {
        // TODO: change to is_evaluating()
        AG::precondition_failure("writing attribute that is not evaluating: %", frame.attribute);
    }

    auto graph = update_stack->graph();
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    graph->value_set_internal(frame.attribute, *frame.attribute.get(), value, *metadata);
}

#pragma mark - Tracing

bool AGGraphIsTracingActive(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    return context->graph().is_tracing_active();
}

void AGGraphPrepareTrace(AGGraphRef graph, void *trace_vtable, void *trace) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().remove_trace(0);

    auto external_trace = new ExternalTrace(reinterpret_cast<ExternalTrace::Interface *>(trace_vtable), trace);
    context->graph().prepare_trace(*external_trace);
}

uint64_t AGGraphAddTrace(AGGraphRef graph, void *interface, void *trace_info) {
    auto context = AG::Graph::Context::from_cf(graph);
    auto trace = new ExternalTrace(reinterpret_cast<ExternalTrace::Interface *>(interface), trace_info);
    context->graph().add_trace(trace);
    return trace->trace_id();
}

void AGGraphRemoveTrace(AGGraphRef graph, uint64_t trace_id) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().remove_trace(trace_id);
}

void AGGraphStartTracing(AGGraphRef graph, uint32_t tracing_flags) { AGGraphStartTracing2(graph, tracing_flags, 0); }

void AGGraphStartTracing2(AGGraphRef graph, uint32_t tracing_flags, uint32_t unknown) {
    if (!graph) {
        AG::Graph::all_start_tracing(tracing_flags, {});
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().start_tracing(tracing_flags, {});
}

void AGGraphStopTracing(AGGraphRef graph) {
    if (!graph) {
        AG::Graph::all_stop_tracing();
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().stop_tracing();
}

void AGGraphSyncTracing(AGGraphRef graph) {
    if (!graph) {
        AG::Graph::all_sync_tracing();
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().sync_tracing();
}

CFStringRef AGGraphCopyTracePath(AGGraphRef graph) {
    if (graph == nullptr) {
        return AG::Graph::all_copy_trace_path();
    }
    auto context = AG::Graph::Context::from_cf(graph);
    return context->graph().copy_trace_path();
}

void AGGraphSetTrace(AGGraphRef graph, void *trace_vtable, void *trace) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().remove_trace(0);

    auto external_trace = new ExternalTrace(0, reinterpret_cast<ExternalTrace::Interface *>(trace_vtable), trace);
    context->graph().add_trace(external_trace);
}

void AGGraphResetTrace(AGGraphRef graph) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().remove_trace(0);
}

bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id) {
    auto context = AG::Graph::Context::from_cf(graph);
    for (auto trace : context->graph().traces()) {
        if (trace->named_event_enabled(event_id)) {
            return true;
        }
    }
    return false;
}

void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, void *value, AGTypeID type) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().foreach_trace([&context, &event_name, &value, &type](AG::Trace &trace) {
        trace.custom_event(*context, event_name, value, *reinterpret_cast<const AG::swift::metadata *>(type));
    });
}

void AGGraphAddNamedTraceEvent(AGGraphRef graph, uint32_t event_id, uint32_t num_event_args, const uint64_t *event_args,
                               CFDataRef data, uint32_t arg6) {
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().foreach_trace([&context, &event_id, &num_event_args, &event_args, &data, &arg6](AG::Trace &trace) {
        trace.named_event(*context, event_id, num_event_args, event_args, data, arg6);
    });
}

const char *AGGraphGetTraceEventName(uint32_t event_id) {
    // TODO: not implemented
    return nullptr;
}

const char *AGGraphGetTraceEventSubsystem(uint32_t event_id) {
    // TODO: not implemented
    return nullptr;
}

void AGGraphRegisterNamedTraceEvent() {
    // TODO: not implemented
}

#pragma mark - Profiler

bool AGGraphIsProfilingEnabled(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    return subgraph->graph()->is_profiling_enabled();
}

uint64_t AGGraphBeginProfileEvent(AGAttribute attribute, const char *event_name) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    auto resolved = attribute_id.resolve(AG::TraversalOptions::AssertNotNil);
    return subgraph->graph()->begin_profile_event(resolved.attribute().to_node_ptr(), event_name);
}

void AGGraphEndProfileEvent(AGAttribute attribute, const char *event_name, uint64_t start_time, bool changed) {
    auto attribute_id = AG::AttributeID(attribute);
    attribute_id.to_node_ptr().assert_valid();

    auto subgraph = attribute_id.subgraph();
    if (!subgraph) {
        AG::precondition_failure("no graph: %u", attribute);
    }

    auto resolved = attribute_id.resolve(AG::TraversalOptions::AssertNotNil);
    subgraph->graph()->end_profile_event(resolved.attribute().to_node_ptr(), event_name, start_time, changed);
}

void AGGraphStartProfiling(AGGraphRef graph) {
    if (!graph) {
        AG::Graph::all_start_profiling(1);
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().start_profiling(1);
}

void AGGraphStopProfiling(AGGraphRef graph) {
    if (!graph) {
        AG::Graph::all_stop_profiling();
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().stop_profiling();
}

void AGGraphMarkProfile(AGGraphRef graph, const char *name, uint64_t time) {
    if (!graph) {
        AG::Graph::all_mark_profile(name);
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    uint32_t event_id = context->graph().intern_key(name);
    context->graph().mark_profile(event_id, time);
}

void AGGraphResetProfile(AGGraphRef graph) {
    if (!graph) {
        AG::Graph::all_reset_profile();
        return;
    }
    auto context = AG::Graph::Context::from_cf(graph);
    context->graph().reset_profile();
}

#pragma mark - Description

void AGGraphDescription(AGGraphRef graph, CFDictionaryRef options) {
    if (graph == nullptr) {
        return AG::Graph::description(nullptr, options);
    }
    auto context = AG::Graph::Context::from_cf(graph);
    return AG::Graph::description(&context->graph(), options);
}

void AGGraphArchiveJSON(const char *filename) { AG::Graph::write_to_file(nullptr, filename, false); }

void AGGraphArchiveJSON2(const char *filename, bool exclude_values) {
    AG::Graph::write_to_file(nullptr, filename, exclude_values);
}
