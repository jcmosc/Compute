#include "Graph.h"

#include <Foundation/Foundation.h>
#include <iostream>

#include "AGGraph.h"
#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/Node/Node.h"
#include "Attribute/OffsetAttributeID.h"
#include "Graph/Profile/ProfileData.h"
#include "Subgraph/Subgraph.h"
#include "Swift/SwiftShims.h"
#include "Time/Time.h"
#include "Trace/Trace.h"
#include "Tree/TreeElement.h"
#include "UpdateStack.h"
#include "Utilities/FreeDeleter.h"
#include "Version/AGVersion.h"

NSString *AGDescriptionFormat = @"format";
NSString *AGDescriptionMaxFrames = @"max-frames";
NSString *AGDescriptionIncludeValues = @"include-values";
NSString *AGDescriptionTruncationLimit = @"truncation-limit";

namespace {

NSString *escaped_string(NSString *string, NSUInteger truncation_limit) {
    if ([string length] > truncation_limit) {
        string = [[string substringToIndex:truncation_limit] stringByAppendingString:@"…"];
    }
    return [string stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
}

} // namespace

namespace AG {

void Graph::print() {
    NSString *description = (__bridge NSString *)description_graph_dot(nil);
    if (description) {
        const char *string = [description UTF8String];
        std::cout << string;
    }
}

void Graph::print_attribute(data::ptr<Node> node) {
    NSString *desc = (__bridge NSString *)description(node);
    if (desc) {
        const char *string = [desc UTF8String];
        fprintf(stdout, "%s\n", string);
    }
}

void Graph::print_data() {
    data::table::shared().print();
    data::zone::print_header();
    for (auto subgraph : _subgraphs) {
        subgraph->data::zone::print();
    }
}

void Graph::print_stack() {
    auto update = current_update();
    if (update != 0) {
        uint32_t update_stack_count = 0;
        for (util::tagged_ptr<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
             update_stack = update_stack.get()->previous()) {
            update_stack_count += 1;
        }
        uint32_t update_stack_index = update_stack_count - 1;
        for (util::tagged_ptr<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
             update_stack = update_stack.get()->previous()) {

            for (auto frame_index = update_stack.get()->frames().size() - 1; frame_index >= 0; --frame_index) {
                auto frame = update_stack.get()->frames()[frame_index];

                uint32_t count = frame.attribute->state().update_count();
                uint32_t input_index = frame.num_pushed_inputs;
                uint32_t num_inputs = frame.attribute->inputs().size();

                const char *pending = frame.needs_update ? "P" : "";
                const char *cancelled = frame.cancelled ? "C" : "";
                const char *dirty = frame.attribute->state().is_dirty() ? "D" : "";
                const char *has_value = frame.attribute->state().is_value_initialized() ? "V" : "";

                fprintf(stdout, "frame %d.%d: attribute %u; count=%d, index=%d/%d %s%s%s%s\n", update_stack_index,
                        (uint32_t)frame_index, frame.attribute.offset(), count, input_index, num_inputs, pending,
                        cancelled, dirty, has_value);
            }

            update_stack_index -= 1;
        }
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
                    [indexSet addIndex:node];
                }

                NSMutableDictionary *dict = [NSMutableDictionary
                    dictionaryWithDictionary:@{AGDescriptionFormat : @"graph/dot", @"attribute-ids" : indexSet}];

                NSString *desc = (NSString *)description(this, (__bridge CFMutableDictionaryRef)dict);
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

#pragma mark - Description

CFStringRef Graph::description(data::ptr<Node> node) {}

id Graph::description(Graph *graph, CFDictionaryRef options) {
    NSString *format = ((__bridge NSDictionary *)options)[AGDescriptionFormat];
    if ([format isEqualToString:@"graph/dict"]) {
        return (__bridge id)description_graph(graph, options);
    }
    if ([format isEqualToString:@"graph/dot"]) {
        return (__bridge id)graph->description_graph_dot(options);
    }
    if ([format hasPrefix:@"stack/"]) {
        auto update = current_update();
        if (auto update_stack = update.get()) {
            if ([format isEqualToString:@"stack/text"]) {
                return (__bridge id)graph->description_stack(options);
            }
            if ([format isEqualToString:@"stack/nodes"]) {
                return (__bridge id)graph->description_stack_nodes(options);
            }
            if ([format isEqualToString:@"stack/frame"]) {
                return (__bridge id)graph->description_stack_frame(options);
            }
        }
    }
    return nil;
}

CFDictionaryRef Graph::description_graph(Graph *graph_param, CFDictionaryRef options) {
    NSNumber *include_values_number = ((__bridge NSDictionary *)options)[AGDescriptionIncludeValues];
    bool include_values = false;
    if (include_values_number) {
        include_values = [include_values_number boolValue];
    }

    NSNumber *truncation_limit_number = ((__bridge NSDictionary *)options)[AGDescriptionTruncationLimit];
    uint64_t truncation_limit = 1024;
    if (truncation_limit_number) {
        truncation_limit = [truncation_limit_number unsignedLongValue];
    }

    NSMutableArray *graph_dicts = [NSMutableArray array];

    auto graph_indices_by_id = std::unordered_map<uint64_t, uint64_t>();
    auto graph_stack = std::stack<Graph *, vector<Graph *, 0, unsigned long>>();

    auto push_graph = [&graph_dicts, &graph_indices_by_id, &graph_stack](Graph *graph) -> uint64_t {
        auto found = graph_indices_by_id.find(graph->unique_id());
        if (found != graph_indices_by_id.end()) {
            return found->second;
        }
        uint64_t index = [graph_dicts count];
        [graph_dicts addObject:[NSNull null]];
        graph_indices_by_id.emplace(graph->unique_id(), index);
        graph_stack.push(graph);
        return index;
    };

    if (graph_param) {
        push_graph(graph_param);
    }
    NSArray *graph_ids = ((__bridge NSDictionary *)options)[@"graph_ids"];
    if (graph_ids && [graph_ids isKindOfClass:[NSArray class]]) {
        all_lock();
        for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
            if ([graph_ids containsObject:[NSNumber numberWithUnsignedLong:graph->unique_id()]]) {
                push_graph(graph);
            }
        }
        all_unlock();
    }
    NSNumber *all_graphs = ((__bridge NSDictionary *)options)[@"all_graphs"];
    if (all_graphs && [all_graphs boolValue]) {
        all_lock();
        for (auto other_graph = _all_graphs; other_graph != nullptr; other_graph = other_graph->_next) {
            if (other_graph != graph_param) {
                push_graph(other_graph);
            }
        }
        all_unlock();
    }

    while (!graph_stack.empty()) {
        Graph *graph = graph_stack.top();
        graph_stack.pop();

        NSMutableArray *node_dicts = [NSMutableArray array];
        NSMutableArray *edge_dicts = [NSMutableArray array];

        auto type_indices_by_id = std::unordered_map<uint32_t, uint64_t>();
        auto type_ids = vector<uint32_t, 0, uint64_t>();

        auto node_indices_by_id = std::unordered_map<data::ptr<Node>, uint64_t>();

        for (auto subgraph : graph->subgraphs()) {
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                uint16_t relative_offset = page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);
                    if (attribute.is_nil()) {
                        break;
                    }

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else {
                        relative_offset = 0;
                    }

                    if (attribute.is_direct()) {
                        uint64_t index = node_indices_by_id.size() + 1;
                        node_indices_by_id.emplace(attribute, index);

                        auto node = attribute.to_node();

                        uint32_t type_id = node.type_id();
                        uint64_t type_index;
                        auto found = type_indices_by_id.find(type_id);
                        if (found != type_indices_by_id.end()) {
                            type_index = found->second;
                        } else {
                            type_index = type_ids.size();
                            type_ids.push_back(type_id);
                            type_indices_by_id.emplace(type_id, index);
                        }

                        NSMutableDictionary *node_dict = [NSMutableDictionary dictionary];
                        node_dict[@"type"] = [NSNumber numberWithUnsignedLong:type_index];
                        node_dict[@"id"] = [NSNumber numberWithUnsignedLong:attribute];

                        const AttributeType &attribute_type = graph->attribute_type(node.type_id());
                        if (node.state().is_self_initialized()) {
                            void *self = node.get_self(attribute_type);
                            if (auto desc = attribute_type.vt_self_description(self)) {
                                node_dict[@"desc"] = escaped_string((__bridge NSString *)desc, truncation_limit);
                            }
                        }
                        if (include_values && node.state().is_value_initialized()) {
                            void *value = node.get_value();
                            if (auto value_desc = attribute_type.vt_value_description(value)) {
                                node_dict[@"value"] = escaped_string((__bridge NSString *)value_desc, truncation_limit);
                            }
                        }

                        auto flags = attribute.to_node().value_state();
                        if (flags) {
                            node_dict[@"flags"] = [NSNumber numberWithUnsignedInt:flags];
                        }

                        auto profile_data = graph->_profile_data.get();
                        if (profile_data) {
                            auto found = profile_data->all_events().items_by_attribute().find(attribute.to_node_ptr());
                            if (found != profile_data->all_events().items_by_attribute().end()) {
                                CFDictionaryRef item_json = profile_data->json_data(found->second, *graph);
                                if (item_json) {
                                    node_dict[@"profile"] = (__bridge NSDictionary *)item_json;
                                }
                            }
                            if (profile_data->categories().size()) {
                                NSMutableDictionary *event_dicts = [NSMutableDictionary dictionary];
                                for (auto &entry : profile_data->categories()) {
                                    uint32_t event_id = entry.first;
                                    auto found = entry.second.items_by_attribute().find(attribute.to_node_ptr());
                                    if (found != entry.second.items_by_attribute().end()) {
                                        CFDictionaryRef item_json = profile_data->json_data(found->second, *graph);
                                        if (item_json) {
                                            NSString *event_name =
                                                [NSString stringWithUTF8String:graph->key_name(event_id)];
                                            event_dicts[event_name] = (__bridge NSDictionary *)item_json;
                                        }
                                    }
                                }
                                if ([event_dicts count]) {
                                    node_dict[@"events"] = event_dicts;
                                }
                            }
                        }

                        [node_dicts addObject:node_dict];
                    }
                }
            }
        }

        for (auto subgraph : graph->subgraphs()) {
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                uint16_t relative_offset = page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);
                    if (attribute.is_nil()) {
                        break;
                    }

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else {
                        relative_offset = 0;
                    }

                    if (attribute.is_direct()) {
                        auto node = attribute.to_node();
                        for (auto input_edge : node.inputs()) {
                            OffsetAttributeID resolved = input_edge.value.resolve(TraversalOptions::None);
                            if (resolved.attribute().is_direct()) {
                                NSMutableDictionary *dict = [NSMutableDictionary dictionary];

                                auto src = node_indices_by_id.find(resolved.attribute().to_node_ptr())->second;
                                dict[@"src"] = [NSNumber numberWithUnsignedLong:src];
                                auto dest = node_indices_by_id.find(attribute.to_node_ptr())->second;
                                dict[@"dest"] = [NSNumber numberWithUnsignedLong:dest];
                                bool indirect = attribute.is_indirect();
                                if (indirect) {
                                    if (resolved.offset()) {
                                        dict[@"offset"] = [NSNumber numberWithUnsignedLong:resolved.offset()];
                                    }
                                }
                                if (indirect || input_edge.is_always_enabled() || input_edge.is_changed() ||
                                    input_edge.is_unknown4() || input_edge.is_unprefetched()) {
                                    dict[@"flags"] = @YES; //
                                }

                                [edge_dicts addObject:dict];
                            }
                        }
                    }
                }
            }
        }

        // Add types for any removed nodes
        if (graph->_profile_data) {
            for (uint32_t type_id = 1; type_id < graph->_types.size(); ++type_id) {
                if (type_indices_by_id.contains(type_id)) {
                    continue;
                }

                auto removed_item = graph->_profile_data->all_events().removed_items_by_type_id().find(type_id);
                if (removed_item != graph->_profile_data->all_events().removed_items_by_type_id().end()) {
                    if (!type_indices_by_id.contains(type_id)) {
                        auto index = type_ids.size();
                        type_ids.push_back(type_id);
                        type_indices_by_id.try_emplace(type_id, index);
                    }
                } else {
                    for (auto entry : graph->_profile_data->categories()) {
                        auto category = entry.second;
                        auto removed_item = category.removed_items_by_type_id().find(type_id);
                        if (removed_item != category.removed_items_by_type_id().end()) {
                            if (!type_indices_by_id.contains(type_id)) {
                                auto index = type_ids.size();
                                type_ids.push_back(type_id);
                                type_indices_by_id.try_emplace(type_id, index);
                            }
                        }
                    }
                }
            }
        }

        NSMutableArray *types = [NSMutableArray array];
        for (auto type_id : type_ids) {
            NSMutableDictionary *dict = [NSMutableDictionary dictionary];

            const AttributeType &type = graph->attribute_type(type_id);

            dict[@"id"] = [NSNumber numberWithUnsignedInt:type_id];
            NSString *name = [NSString stringWithUTF8String:type.self_metadata().name(false)];
            dict[@"name"] = escaped_string(name, truncation_limit);
            NSString *value = [NSString stringWithUTF8String:type.value_metadata().name(false)];
            dict[@"value"] = escaped_string(value, truncation_limit);
            dict[@"size"] =
                [NSNumber numberWithUnsignedLong:type.self_metadata().vw_size() + type.value_metadata().vw_size()];
            dict[@"flags"] = [NSNumber numberWithUnsignedInt:type.flags()];

            auto profile_data = graph->_profile_data.get();
            if (profile_data) {
                auto found = profile_data->all_events().removed_items_by_type_id().find(type_id);
                if (found != profile_data->all_events().removed_items_by_type_id().end()) {
                    CFDictionaryRef item_json = profile_data->json_data(found->second, *graph);
                    if (item_json) {
                        dict[@"profile"] = (__bridge NSDictionary *)item_json;
                    }
                }
                if (profile_data->categories().size()) {
                    NSMutableDictionary *events = [NSMutableDictionary dictionary];
                    for (auto &entry : profile_data->categories()) {
                        uint32_t event_id = entry.first;
                        auto found = entry.second.removed_items_by_type_id().find(type_id);
                        if (found != entry.second.removed_items_by_type_id().end()) {
                            CFDictionaryRef item_json = profile_data->json_data(found->second, *graph);
                            if (item_json) {
                                NSString *event_name = [NSString stringWithUTF8String:graph->key_name(event_id)];
                                events[event_name] = (__bridge NSDictionary *)item_json;
                            }
                        }
                    }
                    if ([events count]) {
                        dict[@"events"] = events;
                    }
                }
            }

            [types addObject:dict];
        }

        // Subgraphs

        auto subgraph_indices_by_id = std::unordered_map<Subgraph *, uint64_t>();
        for (uint32_t index = 0, iter = graph->subgraphs().size(); iter > 0; --iter, ++index) {
            subgraph_indices_by_id.try_emplace(graph->subgraphs()[index], index);
        }

        NSMutableArray *subgraphs = [NSMutableArray array];
        for (auto subgraph : graph->subgraphs()) {
            NSMutableDictionary *dict = [NSMutableDictionary dictionary];
            dict[@"id"] = [NSNumber numberWithUnsignedInt:subgraph->subgraph_id()];
            dict[@"context_id"] = [NSNumber numberWithUnsignedLong:subgraph->context_id()];
            if (subgraph->validation_state() != Subgraph::ValidationState::Valid) {
                dict[@"invalid"] = @YES;
            }

            // Parents
            NSMutableArray *parent_descriptions = [NSMutableArray array];
            for (auto parent : subgraph->parents()) {
                id parent_description;
                auto subgraph_index = subgraph_indices_by_id.find(parent);
                if (subgraph_index != subgraph_indices_by_id.end()) {
                    parent_description = [NSNumber numberWithUnsignedLong:subgraph_index->second];
                } else {
                    uint64_t parent_graph_index;
                    auto parent_graph = graph_indices_by_id.find(parent->graph()->unique_id());
                    if (parent_graph != graph_indices_by_id.end()) {
                        parent_graph_index = parent_graph->second;
                    } else {
                        parent_graph_index = push_graph(parent->graph());
                    }
                    parent_description = @{
                        @"graph" : [NSNumber numberWithUnsignedLong:parent_graph_index],
                        @"subgraph_id" : [NSNumber numberWithUnsignedInt:parent->subgraph_id()]
                    };
                }
                [parent_descriptions addObject:parent_description];
            }
            if ([parent_descriptions count]) {
                dict[@"parents"] = parent_descriptions;
            }

            // Children
            NSMutableArray *child_descriptions = [NSMutableArray array];
            for (auto child : subgraph->children()) {
                Subgraph *child_subgraph = child.subgraph();
                id child_description;
                auto subgraph_index = subgraph_indices_by_id.find(child_subgraph);
                if (subgraph_index != subgraph_indices_by_id.end()) {
                    child_description = [NSNumber numberWithUnsignedLong:subgraph_index->second];
                } else {
                    uint64_t child_graph_index;
                    auto child_graph = graph_indices_by_id.find(child_subgraph->graph()->unique_id());
                    if (child_graph != graph_indices_by_id.end()) {
                        child_graph_index = child_graph->second;
                    } else {
                        child_graph_index = push_graph(child_subgraph->graph());
                    }
                    child_description = @{
                        @"graph" : [NSNumber numberWithUnsignedLong:child_graph_index],
                        @"subgraph_id" : [NSNumber numberWithUnsignedInt:child_subgraph->subgraph_id()]
                    };
                }
                [child_descriptions addObject:child_description];
            }
            if ([child_descriptions count]) {
                dict[@"children"] = child_descriptions;
            }

            // Nodes
            NSMutableArray *nodes = [NSMutableArray array];
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                uint16_t relative_offset = page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);
                    if (attribute.is_nil()) {
                        break;
                    }

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else {
                        relative_offset = 0;
                    }

                    if (attribute.is_direct()) {
                        uint8_t subgraph_flags = attribute.to_node().flags().subgraph_flags();
                        auto found_node_index = node_indices_by_id.find(attribute.to_node_ptr());
                        if (found_node_index != node_indices_by_id.end()) {
                            [nodes addObject:[NSNumber numberWithUnsignedLong:found_node_index->second]];
                            if (subgraph_flags) {
                                NSMutableDictionary *node_dict = nodes[found_node_index->second];
                                node_dict[@"subgraph_flags"] = [NSNumber numberWithUnsignedChar:subgraph_flags];
                                nodes[found_node_index->second] = node_dict;
                            }
                        }
                    }
                }
            }
            if ([nodes count]) {
                dict[@"nodes"] = nodes;
            }

            [subgraphs addObject:dict];
        }

        NSMutableArray *tree_dicts = nil;
        if (auto map = graph->tree_data_elements()) {
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
                        if (found_tree_element == tree_element_indices.end()) {
                            auto index = trees.size();
                            tree_element_indices.try_emplace(tree, index);
                            trees.push_back(tree);

                            if (tree->next) {
                                tree_stack.push(tree->next);
                            }
                            if (tree->old_parent) {
                                tree_stack.push(tree->old_parent);
                            }
                        }
                    }

                    auto found_tree_data_element = map->find(subgraph);
                    if (found_tree_data_element != map->end()) {
                        found_tree_data_element->second.sort_nodes();
                    }
                }

                for (auto tree : trees) {
                    NSMutableDictionary *tree_dict = [NSMutableDictionary dictionary];

                    // TODO: what is creator key?

                    if (tree->owner.without_kind() != 0 && tree->type != nullptr) {
                        OffsetAttributeID resolved = tree->owner.resolve(TraversalOptions::ReportIndirectionInOffset);
                        if (resolved.attribute().is_direct()) {
                            tree_dict[@"node"] = [NSNumber
                                numberWithUnsignedLong:node_indices_by_id.find(resolved.attribute().to_node_ptr())
                                                           ->second];
                            if (resolved.offset() != 0) {
                                tree_dict[@"offset"] = [NSNumber numberWithUnsignedLong:resolved.offset() - 1]; // 3
                            }
                        }

                        tree_dict[@"desc"] = escaped_string([NSString stringWithUTF8String:tree->type->name(false)],
                                                            truncation_limit); // 4

                        if (tree->parent == nullptr) {
                            tree_dict[@"root"] = @YES;
                        }
                    } else if (tree->owner.without_kind() != 0 && tree->type == nullptr) {
                        tree_dict[@"node"] = [NSNumber
                            numberWithUnsignedLong:node_indices_by_id.find(tree->owner.to_node_ptr())->second]; // 2
                    } else {
                        if (tree->parent == nullptr) {
                            tree_dict[@"root"] = @YES; // 1
                        }
                    }

                    if (tree->flags) {
                        tree_dict[@"flags"] = [NSNumber numberWithUnsignedInt:tree->flags];
                    }

                    if (tree->next) {
                        NSMutableArray *children = [NSMutableArray array];
                        for (data::ptr<Graph::TreeElement> child = tree->next; child != nullptr;
                             child = child->old_parent) {
                            [children
                                addObject:[NSNumber numberWithUnsignedLong:tree_element_indices.find(child)->second]];
                        }
                        tree_dict[@"children"] = children;
                    }

                    Subgraph *subgraph = TreeElementID(tree).subgraph();
                    auto tree_data_element = map->find(subgraph);
                    if (tree_data_element != map->end()) {

                        auto data_nodes = tree_data_element->second.nodes();
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
                    for (data::ptr<TreeValue> value = tree->last_value; value != nullptr;
                         value = value->previous_sibling) {

                        OffsetAttributeID resolved_value = value->value.resolve(TraversalOptions::None);
                        if (resolved_value.attribute().is_direct()) {
                            NSMutableDictionary *dict = [NSMutableDictionary dictionary];
                            dict[@"node"] =
                                [NSNumber numberWithUnsignedLong:node_indices_by_id
                                                                     .find(resolved_value.attribute().to_node_ptr())
                                                                     ->second];
                            if (resolved_value.offset() != 0) {
                                dict[@"offset"] = [NSNumber numberWithUnsignedLong:resolved_value.offset()];
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
        graph_dict[@"id"] = [NSNumber numberWithUnsignedLong:graph->unique_id()];
        graph_dict[@"types"] = types;
        graph_dict[@"nodes"] = node_dicts;
        graph_dict[@"edges"] = edge_dicts;
        graph_dict[@"subgraphs"] = subgraphs;
        if (tree_dicts) {
            graph_dict[@"trees"] = tree_dicts;
        }

        graph_dict[@"transaction_count"] = [NSNumber numberWithUnsignedLong:graph->transaction_count()];
        if (auto profile_data = graph->_profile_data.get()) {
            NSDictionary *json = (__bridge NSDictionary *)profile_data->json_data(profile_data->all_events(), *graph);
            if (json) {
                [graph_dict addEntriesFromDictionary:json];
            }
            if (!profile_data->categories().empty()) {
                NSMutableDictionary *events = [NSMutableDictionary dictionary];
                for (auto category : profile_data->categories()) {
                    NSDictionary *json = (__bridge NSDictionary *)profile_data->json_data(category.second, *graph);
                    if (json) {
                        events[[NSString stringWithUTF8String:graph->key_name(category.first)]] = json;
                    }
                }
                if ([events count]) {
                    graph_dict[@"events"] = events;
                }
            }
        } else {
            graph_dict[@"update_count"] = [NSNumber numberWithUnsignedLong:graph->update_count()];
            graph_dict[@"change_count"] = [NSNumber numberWithUnsignedLong:graph->change_count()];
        }

        graph_dicts[graph_indices_by_id.find(graph->unique_id())->second] = graph_dict;
    }

    NSMutableDictionary *description = [NSMutableDictionary dictionary];
    description[@"version"] = @2; // TODO: check
    description[@"graphs"] = graph_dicts;
    return (__bridge CFDictionaryRef)description;
}

CFStringRef Graph::description_graph_dot(CFDictionaryRef _Nullable options) {
    NSNumber *include_values_number = ((__bridge NSDictionary *)options)[AGDescriptionIncludeValues];
    bool include_values = false;
    if (include_values_number) {
        include_values = [include_values_number boolValue];
    }

    id attribute_ids = ((__bridge NSDictionary *)options)[@"attribute-ids"];
    if (![attribute_ids isKindOfClass:[NSIndexSet class]]) {
        attribute_ids = nil;
    }

    NSNumber *truncation_limit_number = ((__bridge NSDictionary *)options)[AGDescriptionTruncationLimit];
    uint64_t truncation_limit = 40;
    if (truncation_limit_number) {
        truncation_limit = [truncation_limit_number unsignedLongValue];
    }

    NSMutableString *result = [NSMutableString string];

    [result appendString:@"digraph {\n"];

    auto indirect_nodes = std::unordered_map<data::ptr<IndirectNode>, data::ptr<IndirectNode>>();

    if (!subgraphs().empty()) {
        for (auto subgraph : subgraphs()) {
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                uint16_t relative_offset = page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else if (attribute.is_nil()) {
                        break;
                    } else {
                        break;
                    }

                    if (!attribute.is_direct()) {
                        continue;
                    }

                    if (!attribute_ids || [attribute_ids containsIndex:attribute]) {

                        [result appendFormat:@"  _%d[label=\"%d", attribute.value(), attribute.value()];

                        Node &node = attribute.to_node();
                        const AttributeType &node_type = attribute_type(node.type_id());
                        if (node.state().is_self_initialized()) {
                            if (auto callback = node_type.vt_get_self_description_callback()) {
                                void *self = node.get_self(node_type);
                                if (auto desc = callback(&node_type, self)) {
                                    [result appendString:@": "];
                                    [result appendString:escaped_string((__bridge NSString *)desc, truncation_limit)];
                                }
                            }
                        }
                        if (include_values && node.state().is_value_initialized()) {
                            if (auto callback = node_type.vt_get_value_description_callback()) {
                                void *value = node.get_value();
                                if (auto value_desc = callback(&node_type, value)) {
                                    [result appendString:@" → "];
                                    [result
                                        appendString:escaped_string((__bridge NSString *)value_desc, truncation_limit)];
                                }
                            }
                        }

                        double duration_fraction = 0.0;
                        if (auto profile_data = _profile_data.get()) {
                            auto items_by_attribute = profile_data->all_events().items_by_attribute();
                            auto found_item = items_by_attribute.find(attribute.to_node_ptr());
                            if (found_item != items_by_attribute.end()) {
                                auto item = found_item->second;
                                if (item.data().update_count) {
                                    uint64_t update_count = item.data().update_count;
                                    uint64_t update_total = item.data().update_total;
                                    uint64_t all_update_total = profile_data->all_events().data().update_total;
                                    double average_update_time =
                                        absolute_time_to_seconds((double)update_total / (double)update_count);
                                    duration_fraction =
                                        ((double)update_total / (double)all_update_total) * 100.0; // duration fraction
                                    [result appendFormat:@"\\n%.2g%%: %g × %.2fμs", duration_fraction,
                                                         (double)update_count, average_update_time * 1000000.0];
                                }
                            }
                        }
                        [result appendString:@"\""];

                        bool filled = false;

                        // heat colors
                        if (node.state().is_updating()) {
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

                        if (node.state().is_value_initialized() && !node.inputs().empty() && !node.outputs().empty()) {
                            if (filled) {
                                [result appendFormat:@" style=filled"];
                            }
                        } else {
                            [result appendFormat:node.state().is_value_initialized() ? @" style=\"bold%s\""
                                                                                     : @" style=\"dashed%s\"",
                                                 filled ? ",filled" : ""];
                        }

                        if (node.state().is_dirty()) {
                            [result appendString:@" color=red"];
                        }

                        [result appendString:@"];\n"];

                        for (auto input_edge : node.inputs()) {

                            AttributeID resolved_input_attribute =
                                input_edge.value.resolve(TraversalOptions::None).attribute();
                            if (resolved_input_attribute.is_direct() &&
                                (!attribute_ids || [attribute_ids containsIndex:resolved_input_attribute.value()])) {

                                [result appendFormat:@"  _%d -> _%d[", input_edge.value.without_kind().value(),
                                                     attribute.value()];

                                // collect source inputs
                                AttributeID intermediate = input_edge.value;
                                while (intermediate.is_indirect()) {
                                    indirect_nodes.try_emplace(intermediate.without_kind().to_indirect_node_ptr(),
                                                               intermediate.without_kind().to_indirect_node_ptr());

                                    AttributeID source = intermediate.to_indirect_node().source().attribute();
                                    if (source.is_direct()) {
                                        break;
                                    }
                                    intermediate = source.resolve(TraversalOptions::SkipMutableReference).attribute();
                                }

                                if (input_edge.is_changed()) {
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

                                if (input_edge.value.is_indirect()) {
                                    uint32_t offset =
                                        input_edge.value.resolve(TraversalOptions::SkipMutableReference).offset();
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
                indirect_node->source().attribute().resolve(TraversalOptions::SkipMutableReference);
            [result appendFormat:@"  _%d -> _%d[label=\"@%d\"];\n", resolved_source.attribute().without_kind().value(),
                                 indirect_node.offset(), resolved_source.offset()];

            if (indirect_node->is_mutable()) {
                if (auto dependency = indirect_node->to_mutable().dependency()) {
                    [result appendFormat:@"  _%d -> _%d[color=blue];\n", dependency.value(), indirect_node.offset()];
                }
            }
        }
    }

    [result appendString:@"}\n"];

    return (__bridge CFStringRef)result;
}

CFStringRef Graph::description_stack(CFDictionaryRef options) {

    NSMutableString *description = [NSMutableString string];

    NSNumber *max_frames_number = [(__bridge NSDictionary *)options objectForKeyedSubscript:AGDescriptionMaxFrames];
    int max_frames = max_frames_number ? [max_frames_number unsignedIntValue] : -1;

    int frame_count = 0;
    for (auto update = current_update(); update != nullptr; update = update.get()->previous()) {
        auto frames = update.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {

            const AttributeType &type = attribute_type(frame->attribute->type_id());
            [description appendFormat:@"  #%d: %u %s -> %s\n", frame_count, frame->attribute.offset(),
                                      type.self_metadata().name(false), type.value_metadata().name(false)];

            if (frame_count == 0 && frame->attribute->inputs().size() > 0) {
                [description appendString:@"  -- inputs:\n"];
                for (auto input_edge : frame->attribute->inputs()) {
                    OffsetAttributeID resolved = input_edge.value.resolve(TraversalOptions::ReportIndirectionInOffset);
                    [description appendFormat:@"    %u", resolved.attribute().value()];
                    if (resolved.offset() != 0) {
                        [description appendFormat:@"[@%d]", resolved.offset() - 1];
                    }
                    if (resolved.attribute().is_direct()) {
                        const AttributeType &input_type = attribute_type(resolved.attribute().to_node().type_id());
                        [description appendFormat:@" %s -> %s", input_type.self_metadata().name(false),
                                                  input_type.value_metadata().name(false)];
                    }
                    if (input_edge.is_changed()) {
                        [description appendString:@", changed"];
                    }
                    if (input_edge.is_always_enabled()) {
                        [description appendString:@", always-enabled"];
                    }
                    if (input_edge.is_unprefetched()) {
                        [description appendString:@", unprefetched"];
                    }
                    [description appendString:@"\n"];
                }
                [description appendString:@"  --\n"];
            }

            frame_count += 1;
            if (frame_count >= max_frames) {
                return (__bridge CFStringRef)description;
            }
        }
    }

    return (__bridge CFStringRef)description;
}

CFArrayRef Graph::description_stack_nodes(CFDictionaryRef options) {
    NSMutableArray *nodes = [NSMutableArray array];

    NSNumber *max_frames_number = [(__bridge NSDictionary *)options objectForKeyedSubscript:AGDescriptionMaxFrames];
    int max_frames = max_frames_number ? [max_frames_number unsignedIntValue] : -1;

    int frame_count = 0;
    for (auto update = current_update(); update != nullptr; update = update.get()->previous()) {
        auto frames = update.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {

            [nodes addObject:[NSNumber numberWithUnsignedInt:frame->attribute]];

            frame_count += 1;
            if (frame_count >= max_frames) {
                return (__bridge CFArrayRef)nodes;
            }
        }
    }

    return (__bridge CFArrayRef)nodes;
}

CFDictionaryRef Graph::description_stack_frame(CFDictionaryRef options) {
    NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];

    NSNumber *frame_index_number = [(__bridge NSDictionary *)options objectForKeyedSubscript:@"frame_index"];
    int frame_index = frame_index_number ? [frame_index_number unsignedIntValue] : -1;

    NSNumber *frame_node_number = [(__bridge NSDictionary *)options objectForKeyedSubscript:@"frame_node"];
    data::ptr<Node> frame_node = frame_node_number ? [frame_node_number unsignedIntValue] : 0;

    for (auto update = current_update(); update != nullptr; update = update.get()->previous()) {
        int i = 0;
        auto frames = update.get()->frames();
        for (auto frame : frames) {

            if (i == frame_index || frame.attribute == frame_node) {

                dictionary[@"index"] = [NSNumber numberWithUnsignedLong:i];
                dictionary[@"node-id"] = [NSNumber numberWithUnsignedInt:frame.attribute];

                const AttributeType &type = attribute_type(frame.attribute->type_id());
                AGSetTypeForKey((__bridge CFMutableDictionaryRef)dictionary, (__bridge CFStringRef) @"self-type",
                                &type.self_metadata());
                AGSetTypeForKey((__bridge CFMutableDictionaryRef)dictionary, (__bridge CFStringRef) @"value-type",
                                &type.value_metadata());

                if (!frame.attribute->inputs().empty()) {
                    NSMutableArray *inputs = [NSMutableArray array];

                    for (auto input_edge : frame.attribute->inputs()) {
                        NSMutableDictionary *input_dictionary = [NSMutableDictionary dictionary];

                        input_dictionary[@"id"] = [NSNumber numberWithUnsignedInt:input_edge.value];

                        OffsetAttributeID resolved =
                            input_edge.value.resolve(TraversalOptions::ReportIndirectionInOffset);

                        input_dictionary[@"node"] = [NSNumber numberWithUnsignedInt:resolved.attribute()];
                        if (resolved.offset() != 0) {
                            input_dictionary[@"offset"] = [NSNumber numberWithUnsignedLong:resolved.offset() - 1];
                        }

                        if (resolved.attribute().is_direct()) {
                            const AttributeType &input_type = attribute_type(resolved.attribute().to_node().type_id());
                            AGSetTypeForKey((__bridge CFMutableDictionaryRef)input_dictionary,
                                            (__bridge CFStringRef) @"self-type", &input_type.self_metadata());
                            AGSetTypeForKey((__bridge CFMutableDictionaryRef)input_dictionary,
                                            (__bridge CFStringRef) @"value-type", &input_type.value_metadata());
                        }
                        if (input_edge.is_changed()) {
                            input_dictionary[@"changed"] = @YES;
                        }
                        if (input_edge.is_always_enabled()) {
                            input_dictionary[@"always-enabled"] = @YES;
                        }
                        if (input_edge.is_unprefetched()) {
                            input_dictionary[@"prefetched"] = @NO;
                        }
                    }

                    dictionary[@"inputs"] = inputs;
                }

                return (__bridge CFDictionaryRef)dictionary;
            }

            i += 1;
        }
    }

    return (__bridge CFDictionaryRef)dictionary;
}

void Graph::write_to_file(Graph *graph, const char *_Nullable filename, bool exclude_values) {
    NSDictionary *options = @{
        AGDescriptionFormat : @"graph/dict",
        AGDescriptionIncludeValues : @(!exclude_values),
        @"all_graphs" : @(graph == nullptr)
    };
    NSDictionary *json = description(graph, (__bridge CFDictionaryRef)options);
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
