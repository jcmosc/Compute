#include "AGGraph-Private.h"

#import <Foundation/Foundation.h>

#include "Context.h"
#include "Graph.h"

#pragma mark - Description

CFTypeRef AGGraphDescription(AGGraphRef graph, CFDictionaryRef options) {
    if (graph == nullptr) {
        return (__bridge CFTypeRef)AG::Graph::description(nullptr, (__bridge NSDictionary *)options);
    }

    auto graph_context = AG::Graph::Context::from_cf(graph);
    if (graph_context->invalidated()) {
        AG::precondition_failure("invalidated graph");
    }
    return (__bridge CFTypeRef)AG::Graph::description(&graph_context->graph(), (__bridge NSDictionary *)options);
}
