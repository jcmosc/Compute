#include "Graph.h"

#if TARGET_OS_MAC

#import <Foundation/Foundation.h>
#include <iostream>
#include <ranges>

#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Attribute/AttributeView/AttributeView.h"
#include "ComputeCxx/AGDescription.h"
#include "Graph/UpdateStack.h"
#include "Subgraph/Subgraph.h"
#include "Swift/SwiftShims.h"
#include "Trace/Trace.h"

namespace {

NSString *escaped_string(NSString *string, NSUInteger truncation_limit) {
    if ([string length] > truncation_limit) {
        string = [[string substringToIndex:truncation_limit] stringByAppendingString:@"…"];
    }
    return [string stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
}

} // namespace

namespace AG {

#pragma mark - Printing

void Graph::print() {
    NSString *description = description_graph_dot(nil);
    if (description) {
        const char *string = [description UTF8String];
        std::cout << string;
    }
}

void Graph::print_data() {
    data::table::shared().print();
    data::zone::print_header();
    for (auto subgraph : _subgraphs) {
        subgraph->data::zone::print();
    }
}

void Graph::print_attribute(data::ptr<Node> node) {
    NSString *desc = description(node);
    if (desc) {
        const char *string = [desc UTF8String];
        fprintf(stdout, "%s\n", string);
    }
}

namespace {

int cycle_verbosity() {
    const char *print_cycles = getenv("AG_PRINT_CYCLES");
    if (print_cycles) {
        return atoi(print_cycles);
    }
    return 1;
}

int trap_cycles() {
    const char *trap_cycles = getenv("AG_TRAP_CYCLES");
    if (trap_cycles) {
        return atoi(trap_cycles) != 0;
    }
    return false;
}

} // namespace

void Graph::print_cycle(data::ptr<Node> node) {
    foreach_trace([&node](Trace &trace) { trace.log_message("cycle detected through attribute: %u", node); });

    static int verbosity = cycle_verbosity();
    if (verbosity >= 1) {
        fprintf(stdout, "=== AttributeGraph: cycle detected through attribute %u ===\n", node.offset());
        if (verbosity >= 2) {
            if (verbosity >= 3) {
                vector<data::ptr<Node>, 0, uint64_t> nodes = {};
                collect_stack(nodes);
                NSMutableIndexSet *indexSet = [NSMutableIndexSet indexSet];
                for (auto node : nodes) {
                    [indexSet addIndex:node.offset()];
                }

                NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:@{
                    (__bridge NSString *)AGDescriptionFormat : @"graph/dot",
                    @"attribute-ids" : indexSet
                }];

                NSString *desc = (NSString *)description(this, dict);
                if (desc) {
                    const char *string = [desc UTF8String];
                    std::cout << string;
                }
                std::cout << "=== Evaluation stack ===\n";
            }

            print_stack();
            std::cout << "===\n";

            if (verbosity >= 4 /* && os_variant_has_internal_diagnostics() */) {
                AGGraphArchiveJSON("cycle.ag-json");
            }
        }
    }

    static bool traps = trap_cycles();
    if (traps) {
        precondition_failure("cyclic graph: %u", node);
    }
}

void Graph::print_stack() {
    auto update = current_update();
    if (update != 0) {
        uint32_t update_stack_count = 0;
        for (util::tagged_ptr<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
             update_stack = update_stack.get()->next()) {
            update_stack_count += 1;
        }
        uint32_t update_stack_index = update_stack_count - 1;
        for (util::tagged_ptr<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
             update_stack = update_stack.get()->next()) {

            for (auto frame_index = update_stack.get()->frames().size() - 1; frame_index >= 0; --frame_index) {
                auto &frame = update_stack.get()->frames()[frame_index];

                uint32_t count = frame.attribute->count();
                uint32_t input_index = frame.num_pushed_inputs;
                uint32_t num_input_edges = frame.attribute->input_edges().size();

                const char *pending = frame.pending ? "P" : "";
                const char *cancelled = frame.cancelled ? "C" : "";
                const char *dirty = frame.attribute->is_dirty() ? "D" : "";
                const char *has_value = frame.attribute->is_value_initialized() ? "V" : "";

                fprintf(stdout, "frame %d.%d: attribute %u; count=%d, index=%d/%d %s%s%s%s\n", update_stack_index,
                        (uint32_t)frame_index, frame.attribute.offset(), count, input_index, num_input_edges, pending,
                        cancelled, dirty, has_value);
            }

            update_stack_index -= 1;
        }
    }
}

#pragma mark - Description

NSString *Graph::description(data::ptr<Node> node) {
    NSMutableArray *array = [NSMutableArray array];

    // TODO: test if keys have spaces or underscores
    [array addObject:[NSString stringWithFormat:@"identifier = %u", node.offset()]];
    [array addObject:[NSString stringWithFormat:@"type = %u", node->type_id()]];

    auto &type = attribute_type(node->type_id());
    [array addObject:[NSString stringWithFormat:@"self_size = %d", (unsigned int)type.body_metadata().vw_size()]];
    [array addObject:[NSString stringWithFormat:@"value_size = %d", (unsigned int)type.value_metadata().vw_size()]];
    if (type.body_metadata().getValueWitnesses()->isPOD()) {
        [array addObject:@"pod_self = true"];
    }
    if (type.value_metadata().getValueWitnesses()->isPOD()) {
        [array addObject:@"pod_value = true"];
    }
    if (type.body_metadata().getValueWitnesses()->isBitwiseTakable()) {
        [array addObject:@"bitwise_takable_self = true"];
    }
    if (type.value_metadata().getValueWitnesses()->isBitwiseTakable()) {
        [array addObject:@"bitwise_takable = true"];
    }
    [array addObject:[NSString stringWithFormat:@"input_count = %d", node->input_edges().size()]];
    [array addObject:[NSString stringWithFormat:@"output_count = %d", node->output_edges().size()]];
    [array addObject:[NSString stringWithFormat:@"dirty = %d", node->is_dirty()]];
    [array addObject:[NSString stringWithFormat:@"updating = %d", node->count()]]; // TODO: check is count and not bool

#if TARGET_OS_MAC
    if (auto selfDescription = type.vtable().self_description) {
        if (node->is_self_initialized()) {
            void *body = node->get_self(type);
            if (auto desc = selfDescription(reinterpret_cast<const AGAttributeType *>(&type), body)) {
                [array addObject:[NSString stringWithFormat:@"self = %@", desc]];
            }
        }
    }
    if (auto valueDescription = type.vtable().value_description) {
        if (node->is_value_initialized()) {
            void *value = node->get_value();
            if (auto desc = valueDescription(reinterpret_cast<const AGAttributeType *>(&type), value)) {
                [array addObject:[NSString stringWithFormat:@"value = %@", desc]];
            }
        }
    }
#else
    if (auto copySelfDescription = type.vtable().copy_self_description) {
        if (node->is_self_initialized()) {
            void *body = node->get_self(type);
            if (auto desc = copySelfDescription(reinterpret_cast<const AGAttributeType *>(&type), body)) {
                [array addObject:[NSString stringWithFormat:@"self = %@", desc]];
                CFRelease(desc);
            }
        }
    }
    if (auto copyValueDescription = type.vtable().copy_value_description) {
        if (node->is_value_initialized()) {
            void *value = node->get_value();
            if (auto desc = copyValueDescription(reinterpret_cast<const AGAttributeType *>(&type), value)) {
                [array addObject:[NSString stringWithFormat:@"value = %@", desc]];
                CFRelease(desc);
            }
        }
    }
#endif

    // TODO: test what actual deliminator is
    return [array componentsJoinedByString:@","];
}

NSObject *Graph::description(Graph *graph, NSDictionary *options) {
    NSString *format = options[(__bridge NSString *)AGDescriptionFormat];
    if ([format isEqualToString:@"graph/dict"]) {
        return description_graph(graph, options);
    }
    if (graph && [format isEqualToString:@"graph/dot"]) {
        return graph->description_graph_dot(options);
    }
    if ([format hasPrefix:@"stack/"]) {
        if (auto update = current_update().get()) {
            if ([format isEqualToString:@"stack/text"]) {
                return graph->description_stack(options);
            }
            if ([format isEqualToString:@"stack/nodes"]) {
                return graph->description_stack_nodes(options);
            }
            if ([format isEqualToString:@"stack/frame"]) {
                return Graph::description_stack_frame(options); // double check is static
            }
        }
    }
    return nil;
}

NSDictionary *Graph::description_graph(Graph *graph, NSDictionary *options) {
    NSNumber *include_values_number = options[(__bridge NSString *)AGDescriptionIncludeValues];
    bool include_values = false;
    if (include_values_number) {
        include_values = [include_values_number boolValue];
    }

    NSNumber *truncation_limit_number = options[(__bridge NSString *)AGDescriptionTruncationLimit];
    uint64_t truncation_limit = 1024;
    if (truncation_limit_number) {
        truncation_limit = [truncation_limit_number unsignedLongValue];
    }

    // Counters
    NSMutableDictionary *counters_dict = [NSMutableDictionary dictionary];
    counters_dict[@"bytes"] = @(data::table::shared().bytes());
    counters_dict[@"max_bytes"] = @(data::table::shared().max_bytes());

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
    NSArray *graph_ids = options[@"graph_ids"];
    if (graph_ids && [graph_ids isKindOfClass:[NSArray class]]) {
        all_lock();
        for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
            if ([graph_ids containsObject:@(graph->id())]) {
                push_graph(graph);
            }
        }
        all_unlock();
    }
    NSNumber *all_graphs = options[@"all_graphs"];
    if (all_graphs && [all_graphs boolValue]) {
        all_lock();
        for (auto other_graph = _all_graphs; other_graph != nullptr; other_graph = other_graph->_next) {
            if (other_graph != graph) {
                push_graph(other_graph);
            }
        }
        all_unlock();
    }

    // Build each graph dictionary
    while (!graph_stack.empty()) {
        Graph *graph = graph_stack.top();
        graph_stack.pop();

        // Counters
        // TODO what is the difference between created and max counts
        NSMutableDictionary *graph_counters_dict = [NSMutableDictionary dictionary];
        graph_counters_dict[@"nodes"] = @(graph->num_nodes());
        graph_counters_dict[@"created_nodes"] = @(graph->num_nodes_total());
        graph_counters_dict[@"max_nodes"] = @(graph->num_nodes_total());
        graph_counters_dict[@"subgraphs"] = @(graph->num_subgraphs());
        graph_counters_dict[@"created_subgraphs"] = @(graph->num_subgraphs_total());
        graph_counters_dict[@"max_subgraphs"] = @(graph->num_subgraphs_total());
        graph_counters_dict[@"updates"] = @(graph->update_count());
        graph_counters_dict[@"changes"] = @(graph->change_count());
        graph_counters_dict[@"transactions"] = @(graph->transaction_count());

        // Collect types to include in the result
        auto type_indices_by_id = std::unordered_map<uint32_t, uint64_t>();
        auto type_ids = vector<uint32_t, 0, uint64_t>();

        // Build "nodes" array
        auto node_indices_by_id = std::unordered_map<data::ptr<Node>, uint64_t>();
        NSMutableArray *node_dicts = [NSMutableArray array];
        for (auto subgraph : graph->subgraphs()) {
            for (auto page : subgraph->pages()) {
                for (auto attribute : attribute_view(page)) {
                    if (auto node = attribute.get_node()) {
                        uint64_t node_index = node_indices_by_id.size();
                        node_indices_by_id.emplace(node, node_index);

                        uint32_t type_id = node->type_id();
                        uint64_t type_index;
                        auto found = type_indices_by_id.find(type_id);
                        if (found != type_indices_by_id.end()) {
                            type_index = found->second;
                        } else {
                            type_index = type_ids.size();
                            type_ids.push_back(type_id);
                            type_indices_by_id.emplace(type_id, type_index);
                        }

                        NSMutableDictionary *node_dict = [NSMutableDictionary dictionary];
                        node_dict[@"type"] = @(type_index);
                        node_dict[@"id"] = @(node.offset());

                        const AttributeType &attribute_type = graph->attribute_type(type_id);

#if TARGET_OS_MAC
                        if (node->is_self_initialized()) {
                            void *body = node->get_self(attribute_type);
                            if (auto desc = attribute_type.self_description(body)) {
                                node_dict[@"desc"] = escaped_string((__bridge NSString *)desc, truncation_limit);
                            }
                        }

                        if (include_values && node->is_value_initialized()) {
                            void *value = node->get_value();
                            if (auto value_desc = attribute_type.value_description(value)) {
                                node_dict[@"value"] = escaped_string((__bridge NSString *)value_desc, truncation_limit);
                            }
                        }
#else
                        if (node->is_self_initialized()) {
                            void *body = node->get_self(attribute_type);
                            if (auto desc = attribute_type.copy_self_description(body)) {
                                node_dict[@"desc"] = escaped_string((__bridge NSString *)desc, truncation_limit);
                                CFRelease(desc);
                            }
                        }

                        if (include_values && node->is_value_initialized()) {
                            void *value = node->get_value();
                            if (auto value_desc = attribute_type.copy_value_description(value)) {
                                node_dict[@"value"] = escaped_string((__bridge NSString *)value_desc, truncation_limit);
                                CFRelease(value_desc);
                            }
                        }
#endif

                        auto flags = node->flags();
                        if (flags) {
                            node_dict[@"flags"] = @(flags);
                        }

                        [node_dicts addObject:node_dict];
                    }
                }
            }
        }

        // Build "types" array
        NSMutableArray *type_dicts = [NSMutableArray array];
        for (auto type_id : type_ids) {
            NSMutableDictionary *type_dict = [NSMutableDictionary dictionary];

            const AttributeType &type = graph->attribute_type(type_id);

            type_dict[@"id"] = @(type_id);
            NSString *name = [NSString stringWithUTF8String:type.body_metadata().name(false)];
            type_dict[@"name"] = escaped_string(name, truncation_limit);
            NSString *value = [NSString stringWithUTF8String:type.value_metadata().name(false)];
            type_dict[@"value"] = escaped_string(value, truncation_limit);
            type_dict[@"size"] = @(type.body_metadata().vw_size() + type.value_metadata().vw_size());
            type_dict[@"flags"] = @(type.flags());

            [type_dicts addObject:type_dict];
        }

        // Build "edges" array
        NSMutableArray *edge_dicts = [NSMutableArray array];
        for (auto subgraph : graph->subgraphs()) {
            for (auto page : subgraph->pages()) {
                for (auto attribute : attribute_view(page)) {
                    if (auto node = attribute.get_node()) {
                        for (auto &input_edge : node->input_edges()) {
                            OffsetAttributeID resolved = input_edge.attribute.resolve(TraversalOptions::None);
                            if (auto input_node = resolved.attribute().get_node()) {
                                NSMutableDictionary *edge_dict = [NSMutableDictionary dictionary];

                                auto src = node_indices_by_id.find(input_node)->second;
                                edge_dict[@"src"] = @(src);
                                auto dest = node_indices_by_id.find(node)->second;
                                edge_dict[@"dst"] = @(dest);
                                bool indirect = input_edge.attribute.is_indirect_node();
                                if (indirect) {
                                    if (resolved.offset()) {
                                        edge_dict[@"offset"] = @(resolved.offset());
                                    }
                                }
                                if (indirect || input_edge.options & AGInputOptionsAlwaysEnabled ||
                                    input_edge.options & AGInputOptionsChanged ||
                                    input_edge.options & AGInputOptionsEnabled ||
                                    input_edge.options & AGInputOptionsUnprefetched) {
                                    edge_dict[@"flags"] = @(input_edge.options);
                                }

                                [edge_dicts addObject:edge_dict];
                            }
                        }
                    }
                }
            }
        }

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

            // Nodes
            NSMutableArray *subgraph_node_dicts = [NSMutableArray array];
            for (auto page : subgraph->pages()) {
                for (auto attribute : attribute_view(page)) {
                    if (auto node = attribute.get_node()) {
                        AGAttributeFlags subgraph_flags = node->subgraph_flags();
                        auto found_node_index = node_indices_by_id.find(node);
                        if (found_node_index != node_indices_by_id.end()) {
                            [subgraph_node_dicts addObject:@(found_node_index->second)];
                            if (subgraph_flags) {
                                NSMutableDictionary *node_dict = node_dicts[found_node_index->second];
                                node_dict[@"subgraph_flags"] = @(subgraph_flags);
                                node_dicts[found_node_index->second] = node_dict;
                            }
                        }
                    }
                }
            }
            if ([subgraph_node_dicts count]) {
                subgraph_dict[@"nodes"] = subgraph_node_dicts;
            }

            [subgraph_dicts addObject:subgraph_dict];
        }

        NSMutableArray *tree_dicts = nil;
        if (graph->has_tree_data()) {
            tree_dicts = [NSMutableArray array];

            auto tree_stack = std::stack<data::ptr<TreeElement>, vector<data::ptr<TreeElement>, 0, uint64_t>>();

            auto tree_element_indices = std::unordered_map<data::ptr<TreeElement>, uint64_t>();
            auto trees = AG::vector<data::ptr<TreeElement>, 0ul, unsigned long>();

            if (!graph->subgraphs().empty()) {
                for (auto subgraph : graph->subgraphs()) {
                    data::ptr<Graph::TreeElement> tree_root = subgraph->tree_root();
                    if (tree_root) {
                        tree_stack.push(tree_root);
                    }
                    while (!tree_stack.empty()) {
                        data::ptr<Graph::TreeElement> tree = tree_stack.top();
                        tree_stack.pop();

                        uint64_t tree_element_index;
                        auto found_tree_element = tree_element_indices.find(tree);
                        if (found_tree_element != tree_element_indices.end()) {
                            auto index = trees.size();
                            tree_element_indices.try_emplace(tree, index);
                            trees.push_back(tree);

                            if (tree->first_child) {
                                tree_stack.push(tree->first_child);
                            }
                            if (tree->next_sibling) {
                                tree_stack.push(tree->next_sibling);
                            }
                        }
                    }

                    if (auto tree_data = graph->tree_data_element_for_subgraph(subgraph)) {
                        tree_data->sort_nodes();
                    }
                }

                for (auto tree : trees) {
                    NSMutableDictionary *tree_dict = [NSMutableDictionary dictionary];

                    // TODO: what is creator key?

                    if (tree->value && tree->type != nullptr) {
                        OffsetAttributeID resolved = tree->value.resolve(TraversalOptions::ReportIndirectionInOffset);
                        if (resolved.attribute().is_node()) {
                            tree_dict[@"node"] = @(node_indices_by_id.find(resolved.attribute().get_node())->second);
                            if (resolved.offset() != 0) {
                                tree_dict[@"offset"] = @(resolved.offset() - 1);
                            }
                        }

                        tree_dict[@"desc"] =
                            escaped_string([NSString stringWithUTF8String:tree->type->name(false)], truncation_limit);

                        if (tree->parent == nullptr) {
                            tree_dict[@"root"] = @YES;
                        }
                    } else if (tree->value && !tree->value.is_nil() && tree->type == nullptr) {
                        tree_dict[@"node"] = @(node_indices_by_id.find(tree->value.get_node())->second);
                    } else {
                        if (tree->parent == nullptr) {
                            tree_dict[@"root"] = @YES;
                        }
                    }

                    if (tree->flags) {
                        tree_dict[@"flags"] = @(tree->flags);
                    }

                    if (tree->first_child) {
                        NSMutableArray *children = [NSMutableArray array];
                        for (data::ptr<Graph::TreeElement> child = tree->first_child; child != nullptr;
                             child = child->next_sibling) {
                            [children addObject:@(tree_element_indices.find(child)->second)];
                        }
                        tree_dict[@"children"] = children;
                    }

                    Subgraph *subgraph = TreeElementID(tree).subgraph();
                    if (auto tree_data_element = graph->tree_data_element_for_subgraph(subgraph)) {

                        auto &data_nodes = tree_data_element->nodes();
                        std::pair<data::ptr<Graph::TreeElement>, data::ptr<Node>> *found = std::find_if(
                            data_nodes.begin(), data_nodes.end(), [&tree](auto node) { return node.first == tree; });

                        if (found != data_nodes.end()) {
                            NSMutableArray *nodes = [NSMutableArray array];
                            for (auto node = found; node != data_nodes.end(); ++node) {
                                if (node->first != tree) {
                                    break;
                                }
                                if (node->second) {
                                    [nodes addObject:[NSNumber
                                                         numberWithUnsignedLong:node_indices_by_id.find(node->second)
                                                                                    ->second]];
                                }
                            }
                            tree_dict[@"nodes"] = nodes;
                        }
                    }

                    NSMutableDictionary *values = [NSMutableDictionary dictionary];
                    for (data::ptr<TreeValue> value = tree->first_value; value != nullptr; value = value->next) {

                        OffsetAttributeID resolved_value = value->value.resolve(TraversalOptions::None);
                        if (resolved_value.attribute() && resolved_value.attribute().is_node()) {
                            NSMutableDictionary *dict = [NSMutableDictionary dictionary];
                            dict[@"node"] = @(node_indices_by_id.find(resolved_value.attribute().get_node())->second);
                            if (resolved_value.offset() != 0) {
                                dict[@"offset"] = @(resolved_value.offset());
                            }

                            values[[NSString stringWithUTF8String:subgraph->graph()->key_name(value->key_id)]] = dict;
                        }
                    }
                    tree_dict[@"values"] = values;

                    [tree_dicts addObject:tree_dict];
                }
            }
        }

        NSMutableDictionary *graph_dict = [NSMutableDictionary dictionary];
        graph_dict[@"id"] = @(graph->id());
        graph_dict[@"counters"] = graph_counters_dict;

        graph_dict[@"types"] = type_dicts;
        graph_dict[@"nodes"] = node_dicts;
        graph_dict[@"edges"] = edge_dicts;
        graph_dict[@"subgraphs"] = subgraph_dicts;
        if (tree_dicts) {
            graph_dict[@"trees"] = tree_dicts;
        }

        graph_dict[@"transaction_count"] = @(graph->transaction_count());
        graph_dict[@"update_count"] = @(graph->update_count());
        graph_dict[@"change_count"] = @(graph->change_count());

        graph_dicts[graph_indices_by_id.find(graph->id())->second] = graph_dict;
    }

    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    dict[@"version"] = @2;
    dict[@"counters"] = counters_dict;
    dict[@"graphs"] = graph_dicts;
    return dict;
}

NSString *Graph::description_graph_dot(NSDictionary *options) {
    NSNumber *include_values_number = options[(__bridge NSString *)AGDescriptionIncludeValues];
    bool include_values = false;
    if (include_values_number) {
        include_values = [include_values_number boolValue];
    }

    NSIndexSet *attribute_ids = options[@"attribute-ids"];
    if (![attribute_ids isKindOfClass:[NSIndexSet class]]) {
        attribute_ids = nil;
    }

    NSNumber *truncation_limit_number = options[(__bridge NSString *)AGDescriptionTruncationLimit];
    uint64_t truncation_limit = 40;
    if (truncation_limit_number) {
        truncation_limit = [truncation_limit_number unsignedLongValue];
    }

    NSMutableString *result = [NSMutableString string];

    [result appendString:@"digraph {\n"];

    auto indirect_nodes = std::unordered_map<data::ptr<IndirectNode>, data::ptr<IndirectNode>>();

    if (!subgraphs().empty()) {
        for (auto subgraph : subgraphs()) {
            for (auto page : subgraph->pages()) {
                for (auto attribute : attribute_view(page)) {
                    if (!attribute.is_node()) {
                        continue;
                    }

                    if (!attribute_ids || [attribute_ids containsIndex:attribute]) {

                        [result appendFormat:@"  _%d[label=\"%d", AGAttribute(attribute), AGAttribute(attribute)];

                        data::ptr<Node> node = attribute.get_node();
                        const AttributeType &node_type = attribute_type(node->type_id());
                        if (node->is_self_initialized()) {
                            if (auto selfDescription = node_type.vtable().self_description) {
                                void *self = node->get_self(node_type);
                                if (auto desc =
                                        selfDescription(reinterpret_cast<const AGAttributeType *>(&node_type), self)) {
                                    [result appendString:@": "];
                                    [result appendString:escaped_string((__bridge NSString *)desc, truncation_limit)];
                                }
                            }
                        }
                        if (include_values && node->is_value_initialized()) {
                            if (auto valueDescription = node_type.vtable().value_description) {
                                void *value = node->get_value();
                                if (auto value_desc = valueDescription(
                                        reinterpret_cast<const AGAttributeType *>(&node_type), value)) {
                                    [result appendString:@" → "];
                                    [result
                                        appendString:escaped_string((__bridge NSString *)value_desc, truncation_limit)];
                                }
                            }
                        }

                        double duration_fraction = 0.0;
                        //                        if (auto profile_data = _profile_data.get()) {
                        //                            auto &items_by_attribute =
                        //                            profile_data->all_events().items_by_attribute(); auto found_item =
                        //                            items_by_attribute.find(attribute.to_ptr<Node>()); if (found_item
                        //                            != items_by_attribute.end()) {
                        //                                auto &item = found_item->second;
                        //                                if (item.data().update_count) {
                        //                                    uint64_t update_count = item.data().update_count;
                        //                                    uint64_t update_total = item.data().update_total;
                        //                                    uint64_t all_update_total =
                        //                                    profile_data->all_events().data().update_total; double
                        //                                    average_update_time =
                        //                                        absolute_time_to_seconds((double)update_total /
                        //                                        (double)update_count);
                        //                                    duration_fraction =
                        //                                        ((double)update_total / (double)all_update_total) *
                        //                                        100.0; // duration fraction
                        //                                    [result appendFormat:@"\\n%.2g%%: %g × %.2fμs",
                        //                                    duration_fraction,
                        //                                                         (double)update_count,
                        //                                                         average_update_time * 1000000.0];
                        //                                }
                        //                            }
                        //                        }

                        [result appendString:@"\""];

                        bool filled = false;

                        // heat colors
                        if (node->is_updating()) {
                            [result appendString:@" fillcolor=cyan"];
                            filled = true;
                        } else if (duration_fraction > 10.0) {
                            [result appendString:@" fillcolor=orangered"];
                            filled = true;
                        } else if (duration_fraction > 5.0) {
                            [result appendString:@" fillcolor=orange"];
                            filled = true;
                        } else if (duration_fraction > 1.0) {
                            [result appendString:@" fillcolor=yellow"];
                            filled = true;
                        }

                        if (node->is_value_initialized() && !node->input_edges().empty() &&
                            !node->output_edges().empty()) {
                            if (filled) {
                                [result appendFormat:@" style=filled"];
                            }
                        } else {
                            [result appendFormat:node->is_value_initialized() ? @" style=\"bold%s\""
                                                                              : @" style=\"dashed%s\"",
                                                 filled ? ",filled" : ""];
                        }

                        if (node->is_dirty()) {
                            [result appendString:@" color=red"];
                        }

                        [result appendString:@"];\n"];

                        for (auto &input_edge : node->input_edges()) {

                            AttributeID resolved_input_attribute =
                                input_edge.attribute.resolve(TraversalOptions::None).attribute();
                            if (resolved_input_attribute.is_node() &&
                                (!attribute_ids || [attribute_ids containsIndex:resolved_input_attribute])) {

                                [result appendFormat:@"  _%d -> _%d[", (uint32_t)input_edge.attribute,
                                                     AGAttribute(attribute)];

                                // collect source inputs
                                AttributeID intermediate = input_edge.attribute;
                                while (intermediate.is_indirect_node()) {
                                    // TODO: check pointer tag is unset
                                    indirect_nodes.try_emplace(intermediate.get_indirect_node(),
                                                               intermediate.get_indirect_node());

                                    AttributeID source = intermediate.get_indirect_node()->source().identifier();
                                    if (source.is_node()) {
                                        break;
                                    }
                                    intermediate = source.resolve(TraversalOptions::SkipMutableReference).attribute();
                                }

                                if (input_edge.options & AGInputOptionsChanged) {
                                    [result appendString:@" color=red"];
                                }

                                Subgraph *input_subgraph = resolved_input_attribute.subgraph();
                                Subgraph *attribute_subgraph = attribute.subgraph();
                                uint64_t input_context_id = input_subgraph ? input_subgraph->context_id() : 0;
                                uint64_t attribute_context_id =
                                    attribute_subgraph ? attribute_subgraph->context_id() : 0;
                                if (input_context_id != attribute_context_id) {
                                    [result appendString:@" penwidth=2"];
                                }

                                if (input_edge.attribute.is_node()) {
                                    uint32_t offset =
                                        input_edge.attribute.resolve(TraversalOptions::SkipMutableReference).offset();
                                    [result appendFormat:@" label=\"@%d\"", offset];
                                }

                                [result appendString:@"];\n"];
                            }
                        }
                    }
                }
            }
        }

        for (auto entry : indirect_nodes) {
            data::ptr<IndirectNode> indirect_node = entry.first;

            [result appendFormat:@"  _%d[label=\"%d\" shape=box];\n", indirect_node.offset(), indirect_node.offset()];

            OffsetAttributeID resolved_source =
                indirect_node->source().identifier().resolve(TraversalOptions::SkipMutableReference);
            [result appendFormat:@"  _%d -> _%d[label=\"@%d\"];\n",
                                 (uint32_t)resolved_source.attribute(), // TODO: check any pointer tag is unset
                                 indirect_node.offset(), resolved_source.offset()];

            if (indirect_node->is_mutable()) {
                if (auto dependency = indirect_node->to_mutable().dependency()) {
                    [result
                        appendFormat:@"  _%d -> _%d[color=blue];\n", AGAttribute(dependency), indirect_node.offset()];
                }
            }
        }
    }

    [result appendString:@"}\n"];

    return result;
}

NSString *Graph::description_stack(NSDictionary *options) {

    NSMutableString *description = [NSMutableString string];

    NSNumber *max_frames_number = [options objectForKeyedSubscript:(__bridge NSString *)AGDescriptionMaxFrames];
    int max_frames = max_frames_number ? [max_frames_number unsignedIntValue] : -1;

    int frame_count = 0;
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        auto &frames = update.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {

            const AttributeType &type = attribute_type(frame->attribute->type_id());
            [description appendFormat:@"  #%d: %u %s -> %s\n", frame_count, frame->attribute.offset(),
                                      type.body_metadata().name(false), type.value_metadata().name(false)];

            if (frame_count == 0 && frame->attribute->input_edges().size() > 0) {
                [description appendString:@"  -- inputs:\n"];
                for (auto &input_edge : frame->attribute->input_edges()) {
                    OffsetAttributeID resolved =
                        input_edge.attribute.resolve(TraversalOptions::ReportIndirectionInOffset);
                    [description appendFormat:@"    %u", AGAttribute(resolved.attribute())];
                    if (resolved.offset() != 0) {
                        [description appendFormat:@"[@%d]", resolved.offset() - 1];
                    }
                    if (resolved.attribute().is_node()) {
                        const AttributeType &input_type = attribute_type(resolved.attribute().get_node()->type_id());
                        [description appendFormat:@" %s -> %s", input_type.body_metadata().name(false),
                                                  input_type.value_metadata().name(false)];
                    }
                    if (input_edge.options & AGInputOptionsChanged) {
                        [description appendString:@", changed"];
                    }
                    if (input_edge.options & AGInputOptionsAlwaysEnabled) {
                        [description appendString:@", always-enabled"];
                    }
                    if (input_edge.options & AGInputOptionsUnprefetched) { // TODO: check is not inverse
                        [description appendString:@", unprefetched"];
                    }
                    [description appendString:@"\n"];
                }
                [description appendString:@"  --\n"];
            }

            frame_count += 1;
            if (frame_count >= max_frames) {
                return description;
            }
        }
    }

    return description;
}

NSArray *Graph::description_stack_nodes(NSDictionary *options) {
    NSMutableArray *nodes = [NSMutableArray array];

    NSNumber *max_frames_number = [options objectForKeyedSubscript:(__bridge NSString *)AGDescriptionMaxFrames];
    int max_frames = max_frames_number ? [max_frames_number unsignedIntValue] : -1;

    int frame_count = 0;
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        auto &frames = update.get()->frames();
        for (auto &frame : std::ranges::reverse_view(update.get()->frames())) {
            [nodes addObject:[NSNumber numberWithUnsignedInt:frame.attribute.offset()]];

            frame_count += 1;
            if (frame_count >= max_frames) {
                return nodes;
            }
        }
    }

    return nodes;
}

NSDictionary *Graph::description_stack_frame(NSDictionary *options) {
    NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];

    NSNumber *frame_index_number = [options objectForKeyedSubscript:@"frame_index"];
    int frame_index = frame_index_number ? [frame_index_number unsignedIntValue] : -1;

    NSNumber *frame_node_number = [options objectForKeyedSubscript:@"frame_node"];
    data::ptr<Node> frame_node = frame_node_number ? [frame_node_number unsignedIntValue] : 0;

    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        auto graph = update.get()->graph(); // double check all frames are part of the same graph

        int i = 0;
        auto &frames = update.get()->frames();
        for (auto &frame : frames) {

            if (i == frame_index || frame.attribute == frame_node) {

                dictionary[@"index"] = @(i);
                dictionary[@"node-id"] = @(frame.attribute.offset());

                const AttributeType &type = graph->attribute_type(frame.attribute->type_id());

                AGSetTypeForKey(dictionary, @"self-type", &type.body_metadata());
                AGSetTypeForKey(dictionary, @"value-type", &type.value_metadata());

                if (!frame.attribute->input_edges().empty()) {
                    NSMutableArray *inputs = [NSMutableArray array];

                    for (auto &input_edge : frame.attribute->input_edges()) {
                        NSMutableDictionary *input_dictionary = [NSMutableDictionary dictionary];

                        input_dictionary[@"id"] = @((uint32_t)input_edge.attribute);

                        OffsetAttributeID resolved =
                            input_edge.attribute.resolve(TraversalOptions::ReportIndirectionInOffset);

                        input_dictionary[@"node"] = @((uint32_t)resolved.attribute());
                        if (resolved.offset() != 0) {
                            input_dictionary[@"offset"] = @(resolved.offset() - 1);
                        }

                        if (resolved.attribute().is_node()) {
                            const AttributeType &input_type =
                                graph->attribute_type(resolved.attribute().get_node()->type_id());
                            AGSetTypeForKey(input_dictionary, @"self-type", &input_type.body_metadata());
                            AGSetTypeForKey(input_dictionary, @"value-type", &input_type.value_metadata());
                        }
                        if (input_edge.options & AGInputOptionsChanged) {
                            input_dictionary[@"changed"] = @YES;
                        }
                        if (input_edge.options & AGInputOptionsAlwaysEnabled) {
                            input_dictionary[@"always-enabled"] = @YES;
                        }
                        if (!(input_edge.options & AGInputOptionsUnprefetched)) { // TODO: check is inverses
                            input_dictionary[@"prefetched"] = @NO;
                        }

                        [inputs addObject:input_dictionary];
                    }

                    dictionary[@"inputs"] = inputs;
                }

                return dictionary;
            }

            i += 1;
        }
    }

    return dictionary;
}

void Graph::write_to_file(Graph *graph, const char *_Nullable filename, bool exclude_values) {
    NSDictionary *options = @{
        (__bridge NSString *)AGDescriptionFormat : @"graph/dict",
        (__bridge NSString *)AGDescriptionIncludeValues : @(!exclude_values),
        @"all_graphs" : @(graph == nullptr)
    };
    NSDictionary *json = (NSDictionary *)description(graph, options);
    if (!json) {
        return;
    }

    if (filename == nullptr) {
        filename = "graph.ag-gzon";
    }

    NSData *data = [NSJSONSerialization dataWithJSONObject:json options:0 error:nil];

    NSString *path = [NSString stringWithUTF8String:filename];
    if (*filename != '/') {
        path = [NSTemporaryDirectory() stringByAppendingPathComponent:path];
    }

    NSError *error = nil;
    if ([[path pathExtension] isEqualToString:@"ag-gzon"]) {
        // Disassembly writes compressed data directly using gzwrite instead of creating an intermediate NSData object
        data = [data compressedDataUsingAlgorithm:NSDataCompressionAlgorithmZlib error:&error];
        if (!data) {
            fprintf(stdout, "Unable to write to \"%s\": %s\n", [path UTF8String], [[error description] UTF8String]);
            return;
        }
    }

    if (![data writeToFile:path options:0 error:&error]) {
        fprintf(stdout, "Unable to write to \"%s\": %s\n", [path UTF8String], [[error description] UTF8String]);
        return;
    }

    fprintf(stdout, "Wrote graph data to \"%s\".\n", [path UTF8String]);
}

} // namespace AG

#endif
