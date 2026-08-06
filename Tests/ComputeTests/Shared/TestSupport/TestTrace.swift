import Foundation

public class TestTrace {
    private var trace: UnsafeMutablePointer<Graph.TraceType>
    private var handle: (Graph, UniqueID)?

    public func install(graph: Graph) {
        guard handle == nil else {
            return
        }
        let traceID = graph.addTrace(trace, context: Unmanaged.passRetained(self as AnyObject).toOpaque())
        self.handle = (graph, traceID)
    }

    public func uninstall() {
        guard let (graph, traceID) = handle else {
            return
        }
        graph.removeTrace(traceID: traceID)
        handle = nil
    }

    public init() {
        self.trace = UnsafeMutablePointer<Graph.TraceType>.allocate(capacity: 1)
        self.trace.initialize(
            to: Graph.TraceType(version: .compareFailed) { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginTrace(graph: graph)
            } end_trace: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endTrace(graph: graph)

                // TODO: Check this calls deinit
                Unmanaged<TestTrace>.fromOpaque(ctx!).release()
            } begin_subgraph_update: { ctx, subgraph, subgraph_flags in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginSubgraphUpdate(subgraph: subgraph, flags: subgraph_flags)
            } end_subgraph_update: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endSubgraphUpdate(subgraph: subgraph)
            } begin_node_update: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginNodeUpdate(attribute: attribute)
            } end_node_update: { ctx, attribute, changed in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                // TODO: test changed or update status
                context.endNodeUpdate(attribute: attribute, changed: changed)
            } begin_value_update: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginValueUpdate(attribute: attribute)
            } end_value_update: { ctx, attribute, changed in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                // TODO: test changed or update status
                context.endValueUpdate(attribute: attribute, changed: changed)
            } begin_graph_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginGraphUpdate(graph: graph)
            } end_graph_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endGraphUpdate(graph: graph)
            } begin_graph_invalidation: { ctx, graph, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginGraphInvalidation(graph: graph, attribute: attribute)
            } end_graph_invalidation: { ctx, graph, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endGraphInvalidation(graph: graph, attribute: attribute)
            } begin_modify_node: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginModifyNode(attribute: attribute)
            } end_modify_node: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endModifyNode(attribute: attribute)
            } begin_event: { ctx, attribute, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.beginEvent(attribute: attribute, eventName: String(cString: event_name))
            } end_event: { ctx, attribute, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.endEvent(attribute: attribute, eventName: String(cString: event_name))
            } graph_created: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.graphCreated(graph: graph)
            } graph_destroy: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.graphDestroy(graph: graph)
            } graph_needs_update: { ctx, graph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.graphNeedsUpdate(graph: graph)
            } subgraph_created: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.subgraphCreated(subgraph: subgraph)
            } subgraph_destroy: { ctx, subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.subgraphDestroy(subgraph: subgraph)
            } subgraph_add_child: { ctx, subgraph, child_subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.subgraphAddChild(subgraph: subgraph, childSubgraph: child_subgraph)
            } subgraph_remove_child: { ctx, subgraph, child_subgraph in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.subgraphRemoveChild(subgraph: subgraph, childSubgraph: child_subgraph)
            } node_added: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeAdded(attribute: attribute)
            } node_add_edge: { ctx, attribute, input, options in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeAddEdge(attribute: attribute, input: input, options: options)
            } node_remove_edge: { ctx, attribute, input in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeRemoveEdge(attribute: attribute, input: input)
            } node_set_edge_pending: { ctx, attribute, input, pending in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeSetEdgePending(attribute: attribute, input: input, pending: pending)
            } node_set_dirty: { ctx, attribute, dirty in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeSetDirty(attribute: attribute, dirty: dirty)
            } node_set_pending: { ctx, attribute, pending in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeSetPending(attribute: attribute, pending: pending)
            } node_set_value: { ctx, attribute, value in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeSetValue(attribute: attribute, value: value)
            } node_mark_value: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.nodeMarkValue(attribute: attribute)
            } indirect_node_added: { ctx, attribute in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.indirectNodeAdded(attribute: attribute)
            } indirect_node_set_source: { ctx, attribute, source in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.indirectNodeSetSource(attribute: attribute, source: source)
            } indirect_node_set_dependency: { ctx, attribute, dependency in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.indirectNodeSetDependency(attribute: attribute, dependency: dependency)
            } profile_mark: { ctx, event_name in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.profileMark(eventName: String(cString: event_name))
            } custom_event: { ctx, graph, event_name, value, type in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.customEvent(graph: graph, eventName: String(cString: event_name), value: value, type: type.type)
            } named_event: { ctx, graph, event_id, event_arg_count, event_args, data, flags in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                let eventArgs = event_args.map { pointer in
                    Array<UInt32>(capacity: event_arg_count) { span in
                        for i in 0..<event_arg_count {
                            span.append(pointer.advanced(by: i).pointee)
                        }
                    }
                }
                context.namedEvent(
                    graph: graph,
                    eventID: event_id,
                    eventArgs: eventArgs ?? [],
                    data: data as Data?,
                    flags: flags
                )
            } named_event_enabled: { ctx, event_id in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                return context.namedEventEnabled(eventID: event_id)
            } set_deadline: { ctx, deadline in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.setDeadline(deadline: UInt(deadline))
            } passed_deadline: { ctx in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.passedDeadline()
            } compare_failed: { ctx, attribute, comparison_state in
                let context = Unmanaged<TestTrace>.fromOpaque(ctx!).takeUnretainedValue()
                context.compareFailed(attribute: attribute, comparisonState: comparison_state)
            }
        )
    }

    deinit {
        self.trace.deallocate()
    }

    public func beginTrace(graph: Graph) {}
    public func endTrace(graph: Graph) {}

    public func beginSubgraphUpdate(subgraph: Subgraph, flags: Subgraph.Flags) {}
    public func endSubgraphUpdate(subgraph: Subgraph) {}
    public func beginNodeUpdate(attribute: AnyAttribute) {}
    public func endNodeUpdate(attribute: AnyAttribute, changed: Bool) {}
    public func beginValueUpdate(attribute: AnyAttribute) {}
    public func endValueUpdate(attribute: AnyAttribute, changed: Bool) {}
    public func beginGraphUpdate(graph: Graph) {}
    public func endGraphUpdate(graph: Graph) {}

    public func beginGraphInvalidation(graph: Graph, attribute: AnyAttribute) {}
    public func endGraphInvalidation(graph: Graph, attribute: AnyAttribute) {}

    public func beginModifyNode(attribute: AnyAttribute) {}
    public func endModifyNode(attribute: AnyAttribute) {}

    public func beginEvent(attribute: AnyAttribute, eventName: String) {}
    public func endEvent(attribute: AnyAttribute, eventName: String) {}

    public func graphCreated(graph: Graph) {}
    public func graphDestroy(graph: Graph) {}
    public func graphNeedsUpdate(graph: Graph) {}

    public func subgraphCreated(subgraph: Subgraph) {}
    public func subgraphDestroy(subgraph: Subgraph) {}
    public func subgraphAddChild(subgraph: Subgraph, childSubgraph: Subgraph) {}
    public func subgraphRemoveChild(subgraph: Subgraph, childSubgraph: Subgraph) {}

    public func nodeAdded(attribute: AnyAttribute) {}
    public func nodeAddEdge(attribute: AnyAttribute, input: AnyAttribute, options: InputOptions) {}
    public func nodeRemoveEdge(attribute: AnyAttribute, input: AnyAttribute) {}
    public func nodeSetEdgePending(attribute: AnyAttribute, input: AnyAttribute, pending: Bool) {}

    public func nodeSetDirty(attribute: AnyAttribute, dirty: Bool) {}
    public func nodeSetPending(attribute: AnyAttribute, pending: Bool) {}
    public func nodeSetValue(attribute: AnyAttribute, value: UnsafeRawPointer) {}
    public func nodeMarkValue(attribute: AnyAttribute) {}

    public func indirectNodeAdded(attribute: AnyAttribute) {}
    public func indirectNodeSetSource(attribute: AnyAttribute, source: AnyAttribute) {}
    public func indirectNodeSetDependency(attribute: AnyAttribute, dependency: AnyAttribute) {}

    public func profileMark(eventName: String) {}
    public func customEvent(graph: Graph, eventName: String, value: UnsafeRawPointer, type: Any.Type) {}
    public func namedEvent(
        graph: Graph,
        eventID: Graph.NamedTraceEventID,
        eventArgs: [UInt32],
        data: Data?,
        flags: Graph.NamedTraceEventFlags
    ) {}
    public func namedEventEnabled(eventID: Graph.NamedTraceEventID) -> Bool { true }

    public func setDeadline(deadline: UInt) {}
    public func passedDeadline() {}

    public func compareFailed(attribute: AnyAttribute, comparisonState: ComparisonState) {}
}
