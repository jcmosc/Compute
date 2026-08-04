import Testing

@Suite
struct GraphTracingTests {
    @Test
    func initializeTraceType() {
        Graph.TraceType(version: .compareFailed) { ctx, graph in
            print("[begin_trace] graph = \(graph)")
        } end_trace: { ctx, graph in
            print("[end_trace] graph = \(graph)")
        } begin_subgraph_update: { ctx, subgraph, options in
            print("[begin_subgraph_update] subgraph = \(subgraph), options = \(options)")
        } end_subgraph_update: { ctx, subgraph in
            print("[end_subgraph_update] subgraph = \(subgraph)")
        } begin_node_update: { ctx, attribute in
            print("[begin_node_update] attribute = \(attribute)")
        } end_node_update: { ctx, attribute, changed in
            print("[end_node_update] attribute = \(attribute), changed = \(changed)")
        } begin_value_update: { ctx, attribute in
            print("[begin_value_update] attribute = \(attribute)")
        } end_value_update: { ctx, attribute, changed in
            print("[end_value_update] attribute = \(attribute), changed = \(changed)")
        } begin_graph_update: { ctx, graph in
            print("[begin_graph_update] graph = \(graph)")
        } end_graph_update: { ctx, graph in
            print("[end_graph_update] graph = \(graph)")
        } begin_graph_invalidation: { ctx, graph, attribute in
            print("[begin_graph_invalidation] graph = \(graph), attribute = \(attribute)")
        } end_graph_invalidation: { ctx, graph, attribute in
            print("[end_graph_invalidation] graph = \(graph), attribute = \(attribute)")
        } begin_modify_node: { ctx, attribute in
            print("[begin_modify_node] attribute = \(attribute)")
        } end_modify_node: { ctx, attribute in
            print("[end_modify_node] attribute = \(attribute)")
        } begin_event: { ctx, attribute, event_name in
            print("[begin_event] attribute = \(attribute), event_name = \(String(cString: event_name))")
        } end_event: { ctx, attribute, event_name in
            print("[end_event] attribute = \(attribute), event_name = \(String(cString: event_name))")
        } graph_created: { ctx, graph in
            print("[graph_created] graph = \(graph)")
        } graph_destroy: { ctx, graph in
            print("[graph_destroy] graph = \(graph)")
        } graph_needs_update: { ctx, graph in
            print("[graph_needs_update] graph = \(graph)")
        } subgraph_created: { ctx, subgraph in
            print("[subgraph_created] subgraph = \(subgraph)")
        } subgraph_destroy: { ctx, subgraph in
            print("[subgraph_destroy] subgraph = \(subgraph)")
        } subgraph_add_child: { ctx, subgraph, child_subgraph in
            print("[subgraph_add_child] subgraph = \(subgraph), child_subgraph = \(child_subgraph)")
        } subgraph_remove_child: { ctx, subgraph, child_subgraph in
            print("[subgraph_remove_child] subgraph = \(subgraph), child_subgraph = \(child_subgraph)")
        } node_added: { ctx, attribute in
            print("[node_added] attribute = \(attribute)")
        } node_add_edge: { ctx, attribute, input, options in
            print("[node_add_edge] attribute = \(attribute), input = \(input), options = \(options)")
        } node_remove_edge: { ctx, attribute, input_index in
            print("[node_remove_edge] attribute = \(attribute), input_index = \(input_index)")
        } node_set_edge_pending: { ctx, attribute, input_index, pending in
            print(
                "[node_set_edge_pending] attribute = \(attribute), input_index = \(input_index), pending = \(pending)"
            )
        } node_set_dirty: { ctx, attribute, dirty in
            print("[node_set_dirty] attribute = \(attribute), dirty = \(dirty)")
        } node_set_pending: { ctx, attribute, pending in
            print("[node_set_pending] attribute = \(attribute), pending = \(pending)")
        } node_set_value: { ctx, attribute, value in
            print("[node_set_value] attribute = \(attribute), value = \(value)")
        } node_mark_value: { ctx, attribute in
            print("[node_mark_value] attribute = \(attribute)")
        } indirect_node_added: { ctx, attribute in
            print("[indirect_node_added] attribute = \(attribute)")
        } indirect_node_set_source: { ctx, attribute, source in
            print("[indirect_node_set_source] attribute = \(attribute), source = \(source)")
        } indirect_node_set_dependency: { ctx, attribute, dependency in
            print("[indirect_node_set_dependency] attribute = \(attribute), dependency = \(dependency)")
        } profile_mark: { ctx, event_name in
            print("[profile_mark] event_name = \(String(cString: event_name))")
        } custom_event: { ctx, graph, event_name, value, type in
            print(
                "[custom_event] graph = \(graph), event_name = \(String(cString: event_name)), value = \(value), type = \(type.type)"
            )
        } named_event: { ctx, graph, event_id, event_arg_count, event_args, data, flags in
            print("[named_event] graph = \(graph), event_id = \(event_id)")
        } named_event_enabled: { ctx, event_id in
            print("[named_event_enabled] event_id = \(event_id)")
            return true
        } set_deadline: { ctx, deadline in
            print("[set_deadline] deadline = \(deadline)")
        } passed_deadline: { ctx in
            print("[passed_deadline]")
        } compare_failed: { ctx, attribute, comparison_state in
            print("[compare_failed] attribute = \(attribute), comparison_state = \(comparison_state)")
        }
    }

    @Test
    func addTrace() {
        class Context {
            var traceCalls: [(name: String, graph: Graph)] = []
        }

        var trace = Graph.TraceType()
        trace.begin_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "beginTrace", graph: graph))
            }
        }
        trace.end_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "endTrace", graph: graph))
            }
        }

        let graph = Graph()
        var context = Context()

        #expect(graph.isTracingActive == false)

        let traceID = withUnsafeMutablePointer(to: &trace) { tracePointer in
            withUnsafeMutablePointer(to: &context) { contextPointer in
                graph.addTrace(tracePointer, context: contextPointer)
            }
        }

        #expect(graph.isTracingActive == true)

        #expect(context.traceCalls.count == 1)
        #expect(context.traceCalls[0].name == "beginTrace")
        #expect(context.traceCalls[0].graph == graph)

        graph.removeTrace(traceID: traceID)

        #expect(context.traceCalls.count == 2)
        #expect(context.traceCalls[1].name == "endTrace")
        #expect(context.traceCalls[1].graph == graph)

        #expect(graph.isTracingActive == false)
    }

    @Test
    func setTrace() {
        class Context {
            var traceCalls: [(name: String, graph: Graph)] = []
        }

        var trace = Graph.TraceType()
        trace.begin_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "beginTrace", graph: graph))
            }
        }
        trace.end_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "endTrace", graph: graph))
            }
        }

        let graph = Graph()
        var context = Context()

        withUnsafeMutablePointer(to: &trace) { tracePointer in
            withUnsafeMutablePointer(to: &context) { contextPointer in
                graph.setTrace(tracePointer, context: contextPointer)
            }
        }

        #expect(context.traceCalls.count == 1)
        #expect(context.traceCalls[0].name == "beginTrace")
        #expect(context.traceCalls[0].graph == graph)

        graph.resetTrace()

        #expect(context.traceCalls.count == 2)
        #expect(context.traceCalls[1].name == "endTrace")
        #expect(context.traceCalls[1].graph == graph)
    }

    @Test
    func namedEvents() throws {
        let eventName = Graph.traceEventName(for: Graph.NamedTraceEventID(rawValue: 0))
        #expect(eventName == nil)

        let eventID = "testname".utf8CString.withUnsafeBufferPointer { namePointer in
            "testsubsystem".utf8CString.withUnsafeBufferPointer { subsystemPointer in
                return Graph.registerNamedTraceEvent(
                    name: namePointer.baseAddress!,
                    subsystem: subsystemPointer.baseAddress!
                )
            }
        }

        let name = try #require(Graph.traceEventName(for: eventID))
        #expect(String(utf8String: name) == "testname")

        let subsystem = try #require(Graph.traceEventSubsystem(for: eventID))
        #expect(String(utf8String: subsystem) == "testsubsystem")
    }
}
