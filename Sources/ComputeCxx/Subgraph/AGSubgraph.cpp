#include "AGSubgraph-Private.h"

#include "Graph/Context.h"
#include "Private/CFRuntime.h"
#include "Subgraph.h"

namespace {

CFRuntimeClass &subgraph_type_id() {
    static auto finalize = [](CFTypeRef subgraph_ref) {
        AGSubgraphStorage *storage = (AGSubgraphStorage *)subgraph_ref;
        AG::Subgraph *subgraph = storage->subgraph;
        if (subgraph) {
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
        NULL, // reclaim
        NULL, // refcount
        0     // requiredAlignment
    };
    return klass;
}

} // namespace

CFTypeID AGSubgraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&subgraph_type_id());
    return type;
}

#pragma mark - Current subgraph

AGSubgraphRef AGSubgraphGetCurrent() {
    auto current = AG::Subgraph::current_subgraph();
    if (current == nullptr) {
        return nullptr;
    }
    return current->to_cf();
}

void AGSubgraphSetCurrent(AGSubgraphRef subgraph) {
    AG::Subgraph *old_subgraph = AG::Subgraph::current_subgraph();
    if (subgraph != nullptr) {
        AG::Subgraph::set_current_subgraph(AG::Subgraph::from_cf(subgraph));
        if (AG::Subgraph::from_cf(subgraph) != nullptr) {
            CFRetain(subgraph);
        }
    } else {
        AG::Subgraph::set_current_subgraph(nullptr);
    }
    if (old_subgraph && old_subgraph->to_cf()) {
        CFRelease(old_subgraph->to_cf());
    }
}

#pragma mark - Graph context

AGSubgraphRef AGSubgraphCreate(AGGraphRef graph) { return AGSubgraphCreate2(graph, AGAttributeNil); };

AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute) {
    CFIndex extra_bytes = sizeof(struct AGSubgraphStorage) - sizeof(CFRuntimeBase);
    AGSubgraphRef instance =
        (AGSubgraphRef)_CFRuntimeCreateInstance(kCFAllocatorDefault, AGSubgraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        AG::precondition_failure("memory allocation failure.");
    }

    AG::Graph::Context *context = AG::Graph::Context::from_cf(graph);

    instance->subgraph = new AG::Subgraph((AG::SubgraphObject *)instance, *context, AG::AttributeID(attribute));
    return instance;
};

AGUnownedGraphRef AGSubgraphGetCurrentGraphContext() {
    AG::Subgraph *current_subgraph = AG::Subgraph::current_subgraph();
    if (current_subgraph == nullptr) {
        return nullptr;
    }

    AG::Graph *graph = current_subgraph->graph();
    return reinterpret_cast<AGUnownedGraphRef>(graph);
}

AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    auto context_id = AG::Subgraph::from_cf(subgraph)->context_id();
    if (context_id != 0) {
        if (auto context = AG::Subgraph::from_cf(subgraph)->graph()->context_with_id(context_id)) {
            return AGGraphContextGetGraph(reinterpret_cast<AGUnownedGraphContextRef>(context));
        }
    }

    AG::precondition_failure("accessing invalidated context");
}

bool AGSubgraphIsValid(AGSubgraphRef subgraph) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return AG::Subgraph::from_cf(subgraph)->is_valid();
}

void AGSubgraphInvalidate(AGSubgraphRef subgraph) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return;
    }

    AG::Subgraph::from_cf(subgraph)->invalidate_and_delete_(false);
}

#pragma mark - Observers

uint64_t AGSubgraphAddObserver(AGSubgraphRef subgraph,
                               void (*observer)(void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                               void *observer_context) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    auto callback = AG::ClosureFunctionVV<void>(observer, observer_context);
    return AG::Subgraph::from_cf(subgraph)->add_observer(callback);
}

void AGSubgraphRemoveObserver(AGSubgraphRef subgraph, uint64_t observer_id) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    AG::Subgraph::from_cf(subgraph)->remove_observer(observer_id);
}

#pragma mark - Children

void AGSubgraphAddChild(AGSubgraphRef subgraph, AGSubgraphRef child) { AGSubgraphAddChild2(subgraph, child, 0); }

void AGSubgraphAddChild2(AGSubgraphRef subgraph, AGSubgraphRef child, uint8_t tag) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    if (AG::Subgraph::from_cf(child) == nullptr) {
        return;
    }

    AG::Subgraph::from_cf(subgraph)->add_child(*AG::Subgraph::from_cf(child), tag);
}

void AGSubgraphRemoveChild(AGSubgraphRef subgraph, AGSubgraphRef child) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    if (child->subgraph) {
        AG::Subgraph::from_cf(subgraph)->remove_child(*AG::Subgraph::from_cf(child), false);
    }
}

AGSubgraphRef AGSubgraphGetChild(AGSubgraphRef subgraph, uint32_t index, uint8_t *tag_out) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    if (index >= AG::Subgraph::from_cf(subgraph)->children().size()) {
        AG::precondition_failure("invalid child index: %u", index);
    }

    AG::Subgraph::SubgraphChild &child = AG::Subgraph::from_cf(subgraph)->children()[index];
    if (tag_out) {
        *tag_out = child.tag();
    }
    return child.subgraph()->to_cf();
}

uint32_t AGSubgraphGetChildCount(AGSubgraphRef subgraph) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }

    return AG::Subgraph::from_cf(subgraph)->children().size();
}

AGSubgraphRef AGSubgraphGetParent(AGSubgraphRef subgraph, int64_t index) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        AG::precondition_failure("accessing invalidated subgraph");
    }
    if (index >= AG::Subgraph::from_cf(subgraph)->parents().size()) {
        AG::precondition_failure("invalid parent index: %u", index);
    }

    return AG::Subgraph::from_cf(subgraph)->parents()[index]->to_cf();
}

uint64_t AGSubgraphGetParentCount(AGSubgraphRef subgraph) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return 0;
    }

    return AG::Subgraph::from_cf(subgraph)->parents().size();
}

bool AGSubgraphIsAncestor(AGSubgraphRef subgraph, AGSubgraphRef other) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }
    if (AG::Subgraph::from_cf(other) == nullptr) {
        return false;
    }

    return AG::Subgraph::from_cf(subgraph)->ancestor_of(*AG::Subgraph::from_cf(other));
}

#pragma mark - Flags

bool AGSubgraphIntersects(AGSubgraphRef subgraph, AGAttributeFlags mask) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return AG::Subgraph::from_cf(subgraph)->intersects(mask);
}

bool AGSubgraphIsDirty(AGSubgraphRef subgraph, AGAttributeFlags mask) {
    if (AG::Subgraph::from_cf(subgraph) == nullptr) {
        return false;
    }

    return AG::Subgraph::from_cf(subgraph)->is_dirty(mask);
}
