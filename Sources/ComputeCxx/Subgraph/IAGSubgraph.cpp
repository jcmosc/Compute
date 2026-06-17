#include "IAGSubgraph-Private.h"

#include <platform/once.h>

#include "Graph/Context.h"
#include "Subgraph.h"

namespace {

CFRuntimeClass &subgraph_type_id() {
    static auto finalize = [](CFTypeRef subgraph_ref) {
        IAGSubgraphStorage *storage = (IAGSubgraphStorage *)subgraph_ref;
        IAG::Subgraph *subgraph = storage->subgraph;
        if (subgraph) {
            subgraph->clear_object();
            subgraph->invalidate_and_delete_(false);
        }
    };
    static CFRuntimeClass klass = {
        0,            // version
        "IAGSubgraph", // className
        NULL,         // init
        NULL,         // copy,
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

CFTypeID IAGSubgraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&subgraph_type_id());
    return type;
}

#pragma mark - Current subgraph

IAGSubgraphRef IAGSubgraphGetCurrent() {
    auto current = IAG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return nullptr;
    }
    return current->to_cf();
}

void IAGSubgraphSetCurrent(IAGSubgraphRef subgraph) {
    IAG::Subgraph *old_subgraph = IAG::Subgraph::current_subgraph();
    if (subgraph != nullptr) {
        IAG::Subgraph::set_current_subgraph(IAG::Subgraph::from_cf(subgraph));
        if (IAG::Subgraph::from_cf(subgraph) != nullptr) {
            CFRetain(subgraph);
        }
    } else {
        IAG::Subgraph::set_current_subgraph(nullptr);
    }
    if (old_subgraph && old_subgraph->to_cf()) {
        CFRelease(old_subgraph->to_cf());
    }
}

#pragma mark - Graph context

IAGSubgraphRef IAGSubgraphCreate(IAGGraphRef graph) { return IAGSubgraphCreate2(graph, IAGAttributeNil); };

IAGSubgraphRef IAGSubgraphCreate2(IAGGraphRef graph, IAGAttribute attribute) {
    CFIndex extra_bytes = sizeof(struct IAGSubgraphStorage) - sizeof(CFRuntimeBase);
    IAGSubgraphRef instance =
        (IAGSubgraphRef)_CFRuntimeCreateInstance(kCFAllocatorDefault, IAGSubgraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        IAG::precondition_failure("memory allocation failure.");
    }

    IAG::Graph::Context *context = IAG::Graph::Context::from_cf(graph);

    instance->subgraph = new IAG::Subgraph((IAG::SubgraphObject *)instance, *context, IAG::AttributeID(attribute));
    return instance;
};

IAGUnownedGraphContextRef IAGSubgraphGetCurrentGraphContext() {
    IAG::Subgraph *current_subgraph = IAG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        return nullptr;
    }

    IAG::Graph *graph = current_subgraph->graph();
    return reinterpret_cast<IAGUnownedGraphContextRef>(graph);
}

IAGGraphRef IAGSubgraphGetGraph(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    auto context_id = IAG::Subgraph::from_cf(subgraph)->context_id();
    if (context_id != 0) {
        if (IAG::Graph::Context *context = IAG::Subgraph::from_cf(subgraph)->graph()->context_with_id(context_id)) {
            return IAGGraphContextGetGraph(context);
        }
    }

    IAG::precondition_failure("accessing invalidated context");
}

bool IAGSubgraphIsValid(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return IAG::Subgraph::from_cf(subgraph)->is_valid();
}

void IAGSubgraphInvalidate(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return;
    }

    IAG::Subgraph::from_cf(subgraph)->invalidate_and_delete_(false);
}

#pragma mark - Index

uint32_t IAGSubgraphGetIndex(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    return IAG::Subgraph::from_cf(subgraph)->index();
}

void IAGSubgraphSetIndex(IAGSubgraphRef subgraph, uint32_t index) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    IAG::Subgraph::from_cf(subgraph)->set_index(index);
}

#pragma mark - Observers

IAGUniqueID IAGSubgraphAddObserver(IAGSubgraphRef subgraph,
                                 void (*observer)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                 const void *observer_context) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    auto callback = IAG::ClosureFunctionVV<void>(observer, observer_context);
    return IAG::Subgraph::from_cf(subgraph)->add_observer(callback);
}

void IAGSubgraphRemoveObserver(IAGSubgraphRef subgraph, IAGUniqueID observer_id) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    IAG::Subgraph::from_cf(subgraph)->remove_observer(observer_id);
}

#pragma mark - Children

void IAGSubgraphAddChild(IAGSubgraphRef subgraph, IAGSubgraphRef child) { IAGSubgraphAddChild2(subgraph, child, 0); }

void IAGSubgraphAddChild2(IAGSubgraphRef subgraph, IAGSubgraphRef child, uint8_t tag) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }
    if (IAG::Subgraph::from_cf(child) == nullptr) {
        return;
    }

    IAG::Subgraph::from_cf(subgraph)->add_child(*IAG::Subgraph::from_cf(child), tag);
}

void IAGSubgraphRemoveChild(IAGSubgraphRef subgraph, IAGSubgraphRef child) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    if (child->subgraph) {
        IAG::Subgraph::from_cf(subgraph)->remove_child(*IAG::Subgraph::from_cf(child), false);
    }
}

IAGSubgraphRef IAGSubgraphGetChild(IAGSubgraphRef subgraph, uint32_t index, uint8_t *tag_out) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }
    if (index >= IAG::Subgraph::from_cf(subgraph)->children().size()) {
        IAG::precondition_failure("invalid child index: %u", index);
    }

    IAG::Subgraph::SubgraphChild &child = IAG::Subgraph::from_cf(subgraph)->children()[index];
    if (tag_out) {
        *tag_out = child.tag();
    }
    return child.subgraph()->to_cf();
}

uint32_t IAGSubgraphGetChildCount(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }

    return IAG::Subgraph::from_cf(subgraph)->children().size();
}

IAGSubgraphRef IAGSubgraphGetParent(IAGSubgraphRef subgraph, int64_t index) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }
    if (index >= IAG::Subgraph::from_cf(subgraph)->parents().size()) {
        IAG::precondition_failure("invalid parent index: %u", index);
    }

    return IAG::Subgraph::from_cf(subgraph)->parents()[index]->to_cf();
}

uint64_t IAGSubgraphGetParentCount(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return 0;
    }

    return IAG::Subgraph::from_cf(subgraph)->parents().size();
}

bool IAGSubgraphIsAncestor(IAGSubgraphRef subgraph, IAGSubgraphRef other) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }
    if (IAG::Subgraph::from_cf(other) == nullptr) {
        return false;
    }

    return IAG::Subgraph::from_cf(subgraph)->ancestor_of(*IAG::Subgraph::from_cf(other));
}

#pragma mark - Flags

bool IAGSubgraphIntersects(IAGSubgraphRef subgraph, IAGAttributeFlags flags) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return IAG::Subgraph::from_cf(subgraph)->intersects(flags);
}

bool IAGSubgraphIsDirty(IAGSubgraphRef subgraph, IAGAttributeFlags flags) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return IAG::Subgraph::from_cf(subgraph)->is_dirty(flags);
}

#pragma mark - Graph

IAGSubgraphRef IAGGraphGetAttributeSubgraph(IAGAttribute attribute) {
    auto subgraph = IAGGraphGetAttributeSubgraph2(attribute);
    if (subgraph == nullptr) {
        IAG::precondition_failure("no subgraph");
    }

    return subgraph;
}

IAGSubgraphRef IAGGraphGetAttributeSubgraph2(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    attribute_id.validate_data_offset();

    auto subgraph = attribute_id.subgraph();
    if (subgraph == nullptr) {
        IAG::precondition_failure("internal error");
    }

    return subgraph->to_cf();
}

void IAGSubgraphApply(IAGSubgraphRef subgraph, uint32_t options,
                     void (*body)(IAGAttribute, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift), const void *body_context) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return;
    }

    IAG::Subgraph::from_cf(subgraph)->apply(options, IAG::ClosureFunctionAV<void, unsigned int>(body, body_context));
}

void IAGSubgraphUpdate(IAGSubgraphRef subgraph, IAGAttributeFlags flags) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return;
    }

    IAG::Subgraph::from_cf(subgraph)->update(flags);
}

#pragma mark - Tree

IAGTreeElement IAGSubgraphGetTreeRoot(IAGSubgraphRef subgraph) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        return IAGTreeElement();
    }

    auto tree_root = IAG::Subgraph::from_cf(subgraph)->tree_root();
    return IAGTreeElement((uintptr_t)tree_root);
}

void IAGSubgraphSetTreeOwner(IAGSubgraphRef subgraph, IAGAttribute owner) {
    if (IAG::Subgraph::from_cf(subgraph) == nullptr) {
        IAG::precondition_failure("accessing invalidated subgraph");
    }
    IAG::Subgraph::from_cf(subgraph)->set_tree_owner(IAG::AttributeID(owner));
}

void IAGSubgraphAddTreeValue(IAGAttribute value, IAGTypeID type, const char *key, uint32_t flags) {
    IAG::Subgraph *current_subgraph = IAG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        return;
    }

    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    current_subgraph->add_tree_value(IAG::AttributeID(value), metadata, key, flags);
}

void IAGSubgraphBeginTreeElement(IAGAttribute value, IAGTypeID type, uint32_t flags) {
    IAG::Subgraph *current_subgraph = IAG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        return;
    }

    auto metadata = reinterpret_cast<const IAG::swift::metadata *>(type);
    current_subgraph->begin_tree(IAG::AttributeID(value), metadata, flags);
}

void IAGSubgraphEndTreeElement(IAGAttribute value) {
    IAG::Subgraph *current_subgraph = IAG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        return;
    }

    current_subgraph->end_tree();
}

static platform_once_t should_record_tree_once = 0;
static bool should_record_tree = true;

void init_should_record_tree() {
    char *result = getenv("IAG_TREE");
    if (result) {
        should_record_tree = atoi(result) != 0;
    } else {
        should_record_tree = false;
    }
}

bool IAGSubgraphShouldRecordTree() {
    platform_once(&should_record_tree_once, init_should_record_tree);
    return should_record_tree;
}

void IAGSubgraphSetShouldRecordTree() {
    platform_once(&should_record_tree_once, init_should_record_tree);
    should_record_tree = true;
}
