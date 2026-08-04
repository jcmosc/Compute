public class TestTrace {
    public enum EventType: Hashable {
        case beginTrace
        case endTrace

        case beginSubgraphUpdate
        case endSubgraphUpdate
        case beginNodeUpdate
        case endNodeUpdate
        case beginValueUpdate
        case endValueUpdate
        case beginGraphUpdate
        case endGraphUpdate

        case beginGraphInvalidation
        case endGraphInvalidation

        case beginModifyNode
        case endModifyNode

        case beginEvent
        case endEvent

        case graphCreated
        case graphDestroy
        case graphNeedsUpdate

        case subgraphCreated
        case subgraphDestroy
        case subgraphAddChild
        case subgraphRemoveChild

        case nodeAdded
        case nodeAddEdge
        case nodeRemoveEdge
        case nodeSetEdgePending

        case nodeSetDirty
        case nodeSetPending
        case nodeSetValue
        case nodeMarkValue

        case indirectNodeAdded
        case indirectNodeSetSource
        case indirectNodeSetDependency

        case profileMark
        case customEvent
        case namedEvent
        case namedEventEnabled

        case setDeadline
        case passedDeadline

        case compareFailed
    }

    var trace: UnsafeMutablePointer<Graph.TraceType>
    var handle: (Graph, UniqueID)?

    public func register(graph: Graph) {
        guard handle == nil else {
            return
        }
        let id = graph.addTrace(trace, context: Unmanaged.passRetained(self as AnyObject).toOpaque())
        self.handle = (graph, id)
    }

    public func unregister() {
        guard let handle else {
            return
        }
        handle.0.removeTrace(traceID: handle.1)
    }

    public struct Event: Hashable {
        public var type: EventType
        public var message: String
    }

    public var events: [Event] = []
    
    public func events(of type: EventType) -> [Event] {
        events.filter { $0.type == type }
    }

    func handleEvent(_ type: EventType, message: String) {
        events.append(Event(type: type, message: message))
    }

    public init() {
        self.trace = UnsafeMutablePointer<Graph.TraceType>.allocate(capacity: 1)
        self.trace.initialize(
            to: Graph.TraceType(version: .compareFailed) { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.beginTrace, message: "graph = \(graph)")
            } end_trace: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.endTrace, message: "graph = \(graph)")

                // Check this calls deinit
                Unmanaged<TestTrace>.fromOpaque(ctx!).release()
            } begin_subgraph_update: { ctx, subgraph, options in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .beginSubgraphUpdate,
                    message: "subgraph = \(subgraph), options = \(options)"
                )
            } end_subgraph_update: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.endSubgraphUpdate, message: "subgraph = \(subgraph)")
            } begin_node_update: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.beginNodeUpdate, message: "attribute = \(attribute)")
            } end_node_update: { ctx, attribute, changed in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .endNodeUpdate,
                    message: "attribute = \(attribute), changed = \(changed)"
                )
            } begin_value_update: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.beginValueUpdate, message: "attribute = \(attribute)")
            } end_value_update: { ctx, attribute, changed in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .endValueUpdate,
                    message: "attribute = \(attribute), changed = \(changed)"
                )
            } begin_graph_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.beginGraphUpdate, message: "graph = \(graph)")
            } end_graph_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.endGraphUpdate, message: "graph = \(graph)")
            } begin_graph_invalidation: { ctx, graph, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .beginGraphInvalidation,
                    message: "graph = \(graph), attribute = \(attribute)"
                )
            } end_graph_invalidation: { ctx, graph, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .endGraphInvalidation,
                    message: "graph = \(graph), attribute = \(attribute)"
                )
            } begin_modify_node: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.beginModifyNode, message: "attribute = \(attribute)")
            } end_modify_node: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.endModifyNode, message: "attribute = \(attribute)")
            } begin_event: { ctx, attribute, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .beginEvent,
                    message: "attribute = \(attribute), event_name = \(String(cString: event_name))"
                )
            } end_event: { ctx, attribute, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .endEvent,
                    message: "attribute = \(attribute), event_name = \(String(cString: event_name))"
                )
            } graph_created: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.graphCreated, message: "graph = \(graph)")
            } graph_destroy: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.graphDestroy, message: "graph = \(graph)")
            } graph_needs_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.graphNeedsUpdate, message: "graph = \(graph)")
            } subgraph_created: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.subgraphCreated, message: "subgraph = \(subgraph)")
            } subgraph_destroy: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.subgraphDestroy, message: "subgraph = \(subgraph)")
            } subgraph_add_child: { ctx, subgraph, child_subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .subgraphAddChild,
                    message: "subgraph = \(subgraph), child_subgraph = \(child_subgraph)"
                )
            } subgraph_remove_child: { ctx, subgraph, child_subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .subgraphRemoveChild,
                    message: "subgraph = \(subgraph), child_subgraph = \(child_subgraph)"
                )
            } node_added: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.nodeAdded, message: "attribute = \(attribute)")
            } node_add_edge: { ctx, attribute, input, options in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeAddEdge,
                    message: "attribute = \(attribute), input = \(input), options = \(options)"
                )
            } node_remove_edge: { ctx, attribute, input_index in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeRemoveEdge,
                    message: "attribute = \(attribute), input_index = \(input_index)"
                )
            } node_set_edge_pending: { ctx, attribute, input_index, pending in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeSetEdgePending,
                    message:
                        "attribute = \(attribute), input_index = \(input_index), pending = \(pending)"
                )
            } node_set_dirty: { ctx, attribute, dirty in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeSetDirty,
                    message: "attribute = \(attribute), dirty = \(dirty)"
                )
            } node_set_pending: { ctx, attribute, pending in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeSetPending,
                    message: "attribute = \(attribute), pending = \(pending)"
                )
            } node_set_value: { ctx, attribute, value in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .nodeSetValue,
                    message: "attribute = \(attribute), value = \(value)"
                )
            } node_mark_value: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.nodeMarkValue, message: "attribute = \(attribute)")
            } indirect_node_added: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.indirectNodeAdded, message: "attribute = \(attribute)")
            } indirect_node_set_source: { ctx, attribute, source in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .indirectNodeSetSource,
                    message: "attribute = \(attribute), source = \(source)"
                )
            } indirect_node_set_dependency: { ctx, attribute, dependency in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .indirectNodeSetDependency,
                    message: "attribute = \(attribute), dependency = \(dependency)"
                )
            } profile_mark: { ctx, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.profileMark, message: "event_name = \(String(cString: event_name))")
            } custom_event: { ctx, graph, event_name, value, type in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .customEvent,
                    message:
                        "graph = \(graph), event_name = \(String(cString: event_name)), value = \(value), type = \(type.type)"
                )
            } named_event: { ctx, graph, event_id, event_arg_count, event_args, data, flags in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.namedEvent, message: "graph = \(graph), event_id = \(event_id)")
            } named_event_enabled: { ctx, event_id in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.namedEventEnabled, message: "event_id = \(event_id)")
                return true
            } set_deadline: { ctx, deadline in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.setDeadline, message: "deadline = \(deadline)")
            } passed_deadline: { ctx in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(.passedDeadline, message: "")
            } compare_failed: { ctx, attribute, comparison_state in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.handleEvent(
                    .compareFailed,
                    message: "attribute = \(attribute), comparison_state = \(comparison_state)"
                )
            }
        )
    }

    deinit {
        self.trace.deallocate()
    }
}
