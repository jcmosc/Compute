#include "Graph.h"

#import <Foundation/Foundation.h>

#include "AGDescription.h"
#include "Subgraph/Subgraph.h"

namespace {

NSString *escaped_string(NSString *string, NSUInteger truncation_limit) {
    if ([string length] > truncation_limit) {
        string = [[string substringToIndex:truncation_limit] stringByAppendingString:@"…"];
    }
    return [string stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
}

} // namespace

namespace AG {

NSObject *Graph::description(Graph *graph, NSDictionary *options) {
    NSString *format = options[(__bridge NSString *)AGDescriptionFormat];
    if ([format isEqualToString:@"graph/dict"]) {
        return description_graph(graph, options);
    }
    return nil;
}

NSDictionary *Graph::description_graph(Graph *graph, NSDictionary *options) {

    NSNumber *truncation_limit_number = options[(__bridge NSString *)AGDescriptionTruncationLimit];
    uint64_t truncation_limit = 1024;
    if (truncation_limit_number) {
        truncation_limit = [truncation_limit_number unsignedLongValue];
    }

    // Build top-level "graphs" array with NSNull placeholders
    NSMutableArray *graph_dicts = [NSMutableArray array];

    // Collect graphs to include in the result
    auto graph_indices_by_id = std::unordered_map<uint64_t, uint64_t>();
    auto graph_stack = std::stack<Graph *, vector<Graph *, 0, unsigned long>>();

    auto push_graph = [&graph_dicts, &graph_indices_by_id, &graph_stack](Graph *graph) -> uint64_t {
        auto found = graph_indices_by_id.find(graph->id());
        if (found != graph_indices_by_id.end()) {
            return found->second;
        }
        uint64_t index = [graph_dicts count];
        [graph_dicts addObject:[NSNull null]];
        graph_indices_by_id.emplace(graph->id(), index);
        graph_stack.push(graph);
        return index;
    };
    if (graph) {
        push_graph(graph);
    }

    // Build each graph dictionary
    while (!graph_stack.empty()) {
        Graph *graph = graph_stack.top();
        graph_stack.pop();

        // Collect types to include in the result
        auto type_ids = vector<uint32_t, 0, uint64_t>();

        // Build "types" array
        NSMutableArray *type_dicts = [NSMutableArray array];
        for (auto type_id : type_ids) {
            NSMutableDictionary *type_dict = [NSMutableDictionary dictionary];

            const AttributeType &type = graph->attribute_type(type_id);

            type_dict[@"id"] = [NSNumber numberWithUnsignedInt:type_id];
            NSString *name = [NSString stringWithUTF8String:type.body_metadata().name(false)];
            type_dict[@"name"] = escaped_string(name, truncation_limit);
            NSString *value = [NSString stringWithUTF8String:type.value_metadata().name(false)];
            type_dict[@"value"] = escaped_string(value, truncation_limit);
            type_dict[@"size"] =
                [NSNumber numberWithUnsignedLong:type.body_metadata().vw_size() + type.value_metadata().vw_size()];
            type_dict[@"flags"] = [NSNumber numberWithUnsignedInt:type.flags()];

            [type_dicts addObject:type_dict];
        }

        // Build "nodes" array
        NSMutableArray *node_dicts = [NSMutableArray array];

        // Build "edges" array
        NSMutableArray *edge_dicts = [NSMutableArray array];

        // Build "subgraphs" array
        NSMutableArray *subgraph_dicts = [NSMutableArray array];

        auto subgraph_indices = std::unordered_map<Subgraph *, uint64_t>();
        for (uint32_t index = 0, iter = graph->subgraphs().size(); iter > 0; --iter, ++index) {
            subgraph_indices.try_emplace(graph->subgraphs()[index], index);
        }

        for (auto subgraph : graph->subgraphs()) {
            NSMutableDictionary *subgraph_dict = [NSMutableDictionary dictionary];
            subgraph_dict[@"id"] = @(subgraph->subgraph_id());
            subgraph_dict[@"context_id"] = @(subgraph->context_id());
            if (!subgraph->is_valid()) {
                subgraph_dict[@"invalid"] = @YES;
            }

            // Parents
            NSMutableArray *parent_dicts = [NSMutableArray array];
            for (Subgraph *parent : subgraph->parents()) {
                NSObject *parent_description;
                auto subgraph_index = subgraph_indices.find(parent);
                if (subgraph_index != subgraph_indices.end()) {
                    parent_description = @(subgraph_index->second);
                } else {
                    uint64_t parent_graph_index = push_graph(parent->graph());
                    parent_description = @{@"graph" : @(parent_graph_index), @"subgraph_id" : @(parent->subgraph_id())};
                }
                [parent_dicts addObject:parent_description];
            }
            if ([parent_dicts count]) {
                subgraph_dict[@"parents"] = parent_dicts;
            }

            // Children
            NSMutableArray *child_dicts = [NSMutableArray array];
            for (auto child : subgraph->children()) {
                NSObject *child_description;
                auto subgraph_index = subgraph_indices.find(child.subgraph());
                if (subgraph_index != subgraph_indices.end()) {
                    child_description = @(subgraph_index->second);
                } else {
                    uint64_t child_graph_index = push_graph(child.subgraph()->graph());
                    child_description =
                        @{@"graph" : @(child_graph_index),
                          @"subgraph_id" : @(child.subgraph()->subgraph_id())};
                }
                [child_dicts addObject:child_description];
            }
            if ([child_dicts count]) {
                subgraph_dict[@"children"] = child_dicts;
            }

            // TODO: nodes

            [subgraph_dicts addObject:subgraph_dict];
        }

        NSMutableDictionary *graph_dict = [NSMutableDictionary dictionary];
        graph_dict[@"id"] = [NSNumber numberWithUnsignedLong:graph->id()];

        graph_dict[@"types"] = type_dicts;
        graph_dict[@"nodes"] = node_dicts;
        graph_dict[@"edges"] = edge_dicts;
        graph_dict[@"subgraphs"] = subgraph_dicts;

        graph_dict[@"transaction_count"] = [NSNumber numberWithUnsignedLong:0];
        graph_dict[@"update_count"] = [NSNumber numberWithUnsignedLong:0];
        graph_dict[@"change_count"] = [NSNumber numberWithUnsignedLong:0];

        graph_dicts[graph_indices_by_id.find(graph->id())->second] = graph_dict;
    }

    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    dict[@"version"] = @2;
    dict[@"graphs"] = graph_dicts;
    return dict;
}

} // namespace AG
