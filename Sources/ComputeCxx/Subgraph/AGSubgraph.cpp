#include "AGSubgraph-Private.h"

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

bool AGSubgraphShouldRecordTree() {
    // TODO: not implemented
    return false;
}
