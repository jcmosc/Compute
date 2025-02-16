#include "Graph.h"

#include <Foundation/Foundation.h>
#include <iostream>

#include "AGGraph.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"
#include "UpdateStack.h"

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
        for (TaggedPointer<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
             update_stack = update_stack.get()->previous()) {
            update_stack_count += 1;
        }
        uint32_t update_stack_index = update_stack_count - 1;
        for (TaggedPointer<Graph::UpdateStack> update_stack = update; update_stack != nullptr;
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

                // TODO: figure out keys and values here
                id objects[2] = {indexSet, indexSet};
                id keys[2] = {@"", @"attributes"};
                NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithObjects:objects forKeys:keys count:2];

                NSString *desc = (__bridge NSString *)description((__bridge CFMutableDictionaryRef)dict);
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

} // namespace AG
