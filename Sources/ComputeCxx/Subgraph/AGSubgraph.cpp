#include "AGSubgraph-Private.h"

#include <dispatch/dispatch.h>

#include "Graph/AGGraph.h"
#include "Graph/Context.h"
#include "Subgraph.h"

namespace {

CFRuntimeClass &subgraph_type_id() {
    static auto finalize = [](CFTypeRef subgraph_ref) {
        AGSubgraphStorage *storage = (AGSubgraphStorage *)subgraph_ref;
        AG::Subgraph *subgraph = storage->subgraph;
        if (subgraph) {
            // TODO: should call destructor?
            subgraph->clear_object();
            subgraph->invalidate_and_delete_(false);
        }
    };
    static CFRuntimeClass klass = {
        0,            // version
        "AGSubgraph", // className
        NULL,         // init
        NULL,         // copy,
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

CFTypeID AGSubgraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&subgraph_type_id());
    return type;
}

AGSubgraphRef AGSubgraphCreate(AGGraphRef graph) { return AGSubgraphCreate2(graph, AGAttributeNil); };

AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute) {
    uint32_t extra = sizeof(struct AGSubgraphStorage) - sizeof(CFRuntimeBase);
    struct AGSubgraphStorage *instance =
        (struct AGSubgraphStorage *)_CFRuntimeCreateInstance(kCFAllocatorDefault, AGSubgraphGetTypeID(), extra, NULL);
    if (!instance) {
        AG::precondition_failure("memory allocation failure.");
    }

    AG::Graph::Context *context = AG::Graph::Context::from_cf(graph);

    AG::Subgraph *subgraph =
        new (&instance->subgraph) AG::Subgraph((AG::SubgraphObject *)instance, *context, AG::AttributeID(attribute));
    return instance;
};

#pragma mark - Current subgraph

AGSubgraphRef AGSubgraphGetCurrent() {
    auto current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return nullptr;
    }
    return current->to_cf();
}

void AGSubgraphSetCurrent(AGSubgraphRef subgraph) {
    // TODO: use util::cf_ptr here?
    AG::Subgraph *old_subgraph = AG::Subgraph::current_subgraph();
    if (subgraph != nullptr) {
        AG::Subgraph::set_current_subgraph(subgraph->subgraph);
        if (subgraph->subgraph != nullptr) {
            CFRetain(subgraph);
        }
    } else {
        AG::Subgraph::set_current_subgraph(nullptr);
    }
    if (old_subgraph && old_subgraph->to_cf()) {
        CFRelease(old_subgraph->to_cf());
    }
}

#pragma mark - Graph

AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    auto context_id = subgraph->subgraph->context_id();
    if (context_id != 0) {
        if (auto context = subgraph->subgraph->graph()->context_with_id(context_id)) {
            return AGGraphContextGetGraph(
                reinterpret_cast<AGUnownedGraphContextRef>(context)); // TODO: proper conversion
        }
    }

    AG::precondition_failure("accessing invalidated contex");
}

AGUnownedGraphContextRef AGSubgraphGetCurrentGraphContext() {
    AG::Subgraph *current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return nullptr;
    }

    // FIXME: this is not the context
    return (AGUnownedGraphContextRef)current->graph();
}

#pragma mark - Children

void AGSubgraphAddChild(AGSubgraphRef subgraph, AGSubgraphRef child) { AGSubgraphAddChild2(subgraph, child, 0); }

void AGSubgraphAddChild2(AGSubgraphRef subgraph, AGSubgraphRef child, uint32_t flags) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    if (child->subgraph != nullptr) {
        // TODO: strong type flags
        subgraph->subgraph->add_child(*child->subgraph, AG::Subgraph::SubgraphChild::Flags(flags));
    }
}

void AGSubgraphRemoveChild(AGSubgraphRef subgraph, AGSubgraphRef child) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    if (child->subgraph) {
        subgraph->subgraph->remove_child(*child->subgraph, false);
    }
}

uint32_t AGSubgraphGetChildCount(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    return subgraph->subgraph->children().size();
}

AGSubgraphRef AGSubgraphGetChild(AGSubgraphRef subgraph, uint32_t index, uint32_t *flags_out) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    if (index >= subgraph->subgraph->children().size()) {
        AG::precondition_failure("invalid child index: %u", index);
    }

    auto child = subgraph->subgraph->children()[index];
    if (flags_out) {
        *flags_out = child.flags();
    }
    return child.subgraph()->to_cf();
}

uint64_t AGSubgraphGetParentCount(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        return 0;
    }

    return subgraph->subgraph->parents().size();
}

AGSubgraphRef AGSubgraphGetParent(AGSubgraphRef subgraph, int64_t index) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    if (index >= subgraph->subgraph->parents().size()) {
        AG::precondition_failure("invalid parent index: %u", index);
    }

    return subgraph->subgraph->parents()[index]->to_cf();
}

bool AGSubgraphIsAncestor(AGSubgraphRef subgraph, AGSubgraphRef possible_descendant) {
    if (subgraph->subgraph == nullptr) {
        return false;
    }

    if (possible_descendant->subgraph == nullptr) {
        return false;
    }

    return subgraph->subgraph->ancestor_of(*possible_descendant->subgraph);
}

#pragma mark - Attributes

bool AGSubgraphIsValid(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        return false;
    }

    return subgraph->subgraph->is_valid();
}

bool AGSubgraphIsDirty(AGSubgraphRef subgraph, uint8_t mask) {
    if (subgraph->subgraph == nullptr) {
        return false;
    }

    return subgraph->subgraph->is_dirty(mask);
}

bool AGSubgraphIntersects(AGSubgraphRef subgraph, uint8_t mask) {
    if (subgraph->subgraph == nullptr) {
        return;
    }

    return subgraph->subgraph->intersects(mask);
}

void AGSubgraphInvalidate(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        return;
    }

    subgraph->subgraph->invalidate_and_delete_(false);
}

void AGSubgraphUpdate(AGSubgraphRef subgraph, uint8_t flags) {
    if (subgraph->subgraph == nullptr) {
        return;
    }

    subgraph->subgraph->update(flags);
}

void AGSubgraphApply(AGSubgraphRef subgraph, uint32_t flags,
                     void (*function)(AGAttribute, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                     const void *function_context) {
    if (subgraph->subgraph == nullptr) {
        return;
    }

    subgraph->subgraph->apply(AG::Subgraph::Flags(flags),
                              AG::ClosureFunctionAV<void, unsigned int>(function, function_context));
}

#pragma mark - Tree

uint32_t AGSubgraphGetTreeRoot(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        return 0; // TODO: nullptr
    }

    return subgraph->subgraph->tree_root();
}

void AGSubgraphBeginTreeElement(AGSubgraphRef subgraph, AGAttribute owner, AGTypeID type, uint32_t flags) {
    AG::Subgraph *current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return;
    }

    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    current->begin_tree(AG::AttributeID(owner), metadata, flags);
}

void AGSubgraphEndTreeElement(AGSubgraphRef subgraph) {
    AG::Subgraph *current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return;
    }

    current->end_tree();
}

void AGSubgraphSetTreeOwner(AGSubgraphRef subgraph, AGAttribute owner) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    subgraph->subgraph->set_tree_owner(AG::AttributeID(owner));
}

void AGSubgraphAddTreeValue(AGSubgraphRef subgraph, AGAttribute attribute, AGTypeID type, const char *key,
                            uint32_t flags) {
    AG::Subgraph *current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return;
    }

    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    current->add_tree_value(AG::AttributeID(attribute), metadata, key, flags);
}

static dispatch_once_t should_record_tree_once = 0;
static bool should_record_tree = true;

void init_should_record_tree(void *context) {
    char *result = getenv("AG_TREE");
    if (result) {
        should_record_tree = atoi(result) != 0;
    } else {
        should_record_tree = false;
    }
}

bool AGSubgraphShouldRecordTree() {
    dispatch_once_f(&should_record_tree_once, nullptr, init_should_record_tree);
    return should_record_tree;
}

void AGSubgraphSetShouldRecordTree() {
    dispatch_once_f(&should_record_tree_once, nullptr, init_should_record_tree);
    should_record_tree = true;
}

#pragma mark - Observers

uint64_t AGSubgraphAddObserver(AGSubgraphRef subgraph,
                               void (*observer)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                               const void *observer_context) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    return subgraph->subgraph->add_observer(AG::ClosureFunctionVV<void>(observer, observer_context));
}

void AGSubgraphRemoveObserver(AGSubgraphRef subgraph, uint64_t observer_id) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    subgraph->subgraph->remove_observer(observer_id);
}

#pragma mark - Index

uint32_t AGSubgraphGetIndex(AGSubgraphRef subgraph) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    return subgraph->subgraph->index();
}

void AGSubgraphSetIndex(AGSubgraphRef subgraph, uint32_t index) {
    if (subgraph->subgraph == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    subgraph->subgraph->set_index(index);
}
