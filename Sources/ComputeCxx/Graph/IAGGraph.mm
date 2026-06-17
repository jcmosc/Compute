#include "IAGGraph-Private.h"

#if TARGET_OS_MAC

#import <Foundation/Foundation.h>

#include "Context.h"
#include "Graph.h"

#pragma mark - Description

CFTypeRef IAGGraphDescription(IAGGraphRef graph, CFDictionaryRef options) {
    if (graph == nullptr) {
        return (__bridge CFTypeRef)IAG::Graph::description(nullptr, (__bridge NSDictionary *)options);
    }

    auto graph_context = IAG::Graph::Context::from_cf(graph);
    if (graph_context->invalidated()) {
        IAG::precondition_failure("invalidated graph");
    }
    return (__bridge CFTypeRef)IAG::Graph::description(&graph_context->graph(), (__bridge NSDictionary *)options);
}

#endif
