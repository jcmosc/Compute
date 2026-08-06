import Foundation

public class TestTraceRecorder: TestTrace {
    public private(set) var history = History()
    
    public override func beginTrace(graph: Graph) {
        history.entries.append(.beginTrace(.init(graph: graph)))
    }
    
    public override func endTrace(graph: Graph) {
        history.entries.append(.endTrace(.init(graph: graph)))
    }
    
    public override func beginSubgraphUpdate(subgraph: Subgraph, flags: Subgraph.Flags) {
        history.entries.append(.beginSubgraphUpdate(.init(subgraph: subgraph, flags: flags)))
    }
    
    public override func endSubgraphUpdate(subgraph: Subgraph) {
        history.entries.append(.endSubgraphUpdate(.init(subgraph: subgraph)))
    }
    
    public override func beginNodeUpdate(attribute: AnyAttribute) {
        history.entries.append(.beginNodeUpdate(.init(attribute: attribute)))
    }
    
    public override func endNodeUpdate(attribute: AnyAttribute, changed: Bool) {
        history.entries.append(.endNodeUpdate(.init(attribute: attribute, changed: changed)))
    }
    
    public override func beginValueUpdate(attribute: AnyAttribute) {
        history.entries.append(.beginValueUpdate(.init(attribute: attribute)))
    }
    
    public override func endValueUpdate(attribute: AnyAttribute, changed: Bool) {
        history.entries.append(.endValueUpdate(.init(attribute: attribute, changed: changed)))
    }
    
    public override func beginGraphUpdate(graph: Graph) {
        history.entries.append(.beginGraphUpdate(.init(graph: graph)))
    }
    
    public override func endGraphUpdate(graph: Graph) {
        history.entries.append(.endGraphUpdate(.init(graph: graph)))
    }
    
    public override func beginGraphInvalidation(graph: Graph, attribute: AnyAttribute) {
        history.entries.append(.beginGraphInvalidation(.init(graph: graph, attribute: attribute)))
    }
    
    public override func endGraphInvalidation(graph: Graph, attribute: AnyAttribute) {
        history.entries.append(.endGraphInvalidation(.init(graph: graph, attribute: attribute)))
    }
    
    public override func beginModifyNode(attribute: AnyAttribute) {
        history.entries.append(.beginModifyNode(.init(attribute: attribute)))
    }
    
    public override func endModifyNode(attribute: AnyAttribute) {
        history.entries.append(.endModifyNode(.init(attribute: attribute)))
    }
    
    public override func beginEvent(attribute: AnyAttribute, eventName: String) {
        history.entries.append(.beginEvent(.init(attribute: attribute, eventName: eventName)))
    }
    
    public override func endEvent(attribute: AnyAttribute, eventName: String) {
        history.entries.append(.endEvent(.init(attribute: attribute, eventName: eventName)))
    }
    
    public override func graphCreated(graph: Graph) {
        history.entries.append(.graphCreated(.init(graph: graph)))
    }
    
    public override func graphDestroy(graph: Graph) {
        history.entries.append(.graphDestroy(.init(graph: graph)))
    }
    
    public override func graphNeedsUpdate(graph: Graph) {
        history.entries.append(.graphNeedsUpdate(.init(graph: graph)))
    }
    
    public override func subgraphCreated(subgraph: Subgraph) {
        history.entries.append(.subgraphCreated(.init(subgraph: subgraph)))
    }
    
    public override func subgraphDestroy(subgraph: Subgraph) {
        history.entries.append(.subgraphDestroy(.init(subgraph: subgraph)))
    }
    
    public override func subgraphAddChild(subgraph: Subgraph, childSubgraph: Subgraph) {
        history.entries.append(.subgraphAddChild(.init(subgraph: subgraph, childSubgraph: childSubgraph)))
    }
    
    public override func subgraphRemoveChild(subgraph: Subgraph, childSubgraph: Subgraph) {
        history.entries.append(.subgraphRemoveChild(.init(subgraph: subgraph, childSubgraph: childSubgraph)))
    }
    
    public override func nodeAdded(attribute: AnyAttribute) {
        history.entries.append(.nodeAdded(.init(attribute: attribute)))
    }
    
    public override func nodeAddEdge(attribute: AnyAttribute, inputAttribute: AnyAttribute, options: InputOptions) {
        history.entries.append(.nodeAddEdge(.init(attribute: attribute, inputAttribute: inputAttribute, options: options)))
    }
    
    public override func nodeRemoveEdge(attribute: AnyAttribute, inputIndex: Int) {
        history.entries.append(.nodeRemoveEdge(.init(attribute: attribute, inputIndex: inputIndex)))
    }
    
    public override func nodeSetEdgePending(attribute: AnyAttribute, inputIndex: Int, pending: Bool) {
        history.entries.append(.nodeSetEdgePending(.init(attribute: attribute, inputIndex: inputIndex, pending: pending)))
    }
    
    public override func nodeSetDirty(attribute: AnyAttribute, dirty: Bool) {
        history.entries.append(.nodeSetDirty(.init(attribute: attribute, dirty: dirty)))
    }
    
    public override func nodeSetPending(attribute: AnyAttribute, pending: Bool) {
        history.entries.append(.nodeSetPending(.init(attribute: attribute, pending: pending)))
    }
    
    public override func nodeSetValue(attribute: AnyAttribute, value: UnsafeRawPointer) {
        history.entries.append(.nodeSetValue(.init(attribute: attribute, value: value)))
    }
    
    public override func nodeMarkValue(attribute: AnyAttribute) {
        history.entries.append(.nodeMarkValue(.init(attribute: attribute)))
    }
    
    public override func indirectNodeAdded(attribute: AnyAttribute) {
        history.entries.append(.indirectNodeAdded(.init(attribute: attribute)))
    }
    
    public override func indirectNodeSetSource(attribute: AnyAttribute, source: AnyAttribute) {
        history.entries.append(.indirectNodeSetSource(.init(attribute: attribute, source: source)))
    }
    
    public override func indirectNodeSetDependency(attribute: AnyAttribute, dependency: AnyAttribute) {
        history.entries.append(.indirectNodeSetDependency(.init(attribute: attribute, dependency: dependency)))
    }
    
    public override func profileMark(eventName: String) {
        history.entries.append(.profileMark(.init(eventName: eventName)))
    }
    
    public override func customEvent(graph: Graph, eventName: String, value: UnsafeRawPointer, type: Any.Type) {
        history.entries.append(.customEvent(.init(graph: graph, eventName: eventName, value: value, type: type)))
    }
    
    public override func namedEvent(
        graph: Graph,
        eventID: Graph.NamedTraceEventID,
        eventArgs: [UInt32],
        data: Data?,
        flags: Graph.NamedTraceEventFlags
    ) {
        history.entries.append(
            .namedEvent(
                .init(
                    graph: graph,
                    eventID: eventID,
                    eventArgs: eventArgs,
                    data: data,
                    flags: flags
                )
            )
        )
    }
    
    public override func namedEventEnabled(eventID: Graph.NamedTraceEventID) {
        history.entries.append(.namedEventEnabled(.init(eventID: eventID)))
    }
    
    public override func setDeadline(deadline: UInt) {
        history.entries.append(.setDeadline(.init(deadline: deadline)))
    }
    
    public override func passedDeadline() {
        history.entries.append(.passedDeadline(.init()))
    }
    
    public override func compareFailed(attribute: AnyAttribute, comparisonState: ComparisonState) {
        history.entries.append(.compareFailed(.init(attribute: attribute, comparisonState: comparisonState)))
    }
}

extension TestTraceRecorder {
    public struct History {
        var entries: [Entry]
        
        init() {
            self.entries = []
        }
        
        public enum Entry {
            case beginTrace(BeginTraceEntry)
            case endTrace(EndTraceEntry)
            
            case beginSubgraphUpdate(BeginSubgraphUpdateEntry)
            case endSubgraphUpdate(EndSubgraphUpdateEntry)
            case beginNodeUpdate(BeginNodeUpdateEntry)
            case endNodeUpdate(EndNodeUpdateEntry)
            case beginValueUpdate(BeginValueUpdateEntry)
            case endValueUpdate(EndValueUpdateEntry)
            case beginGraphUpdate(BeginGraphUpdateEntry)
            case endGraphUpdate(EndGraphUpdateEntry)
            
            case beginGraphInvalidation(BeginGraphInvalidationEntry)
            case endGraphInvalidation(EndGraphInvalidationEntry)
            
            case beginModifyNode(BeginModifyNodeEntry)
            case endModifyNode(EndModifyNodeEntry)
            
            case beginEvent(BeginEventEntry)
            case endEvent(EndEventEntry)
            
            case graphCreated(GraphCreatedEntry)
            case graphDestroy(GraphDestroyEntry)
            case graphNeedsUpdate(GraphNeedsUpdateEntry)
            
            case subgraphCreated(SubgraphCreatedEntry)
            case subgraphDestroy(SubgraphDestroyEntry)
            case subgraphAddChild(SubgraphAddChildEntry)
            case subgraphRemoveChild(SubgraphRemoveChildEntry)
            
            case nodeAdded(NodeAddedEntry)
            case nodeAddEdge(NodeAddEdgeEntry)
            case nodeRemoveEdge(NodeRemoveEdgeEntry)
            case nodeSetEdgePending(NodeSetEdgePendingEntry)
            
            case nodeSetDirty(NodeSetDirtyEntry)
            case nodeSetPending(NodeSetPendingEntry)
            case nodeSetValue(NodeSetValueEntry)
            case nodeMarkValue(NodeMarkValueEntry)
            
            case indirectNodeAdded(IndirectNodeAddedEntry)
            case indirectNodeSetSource(IndirectNodeSetSourceEntry)
            case indirectNodeSetDependency(IndirectNodeSetDependencyEntry)
            
            case profileMark(ProfileMarkEntry)
            case customEvent(CustomEventEntry)
            case namedEvent(NamedEventEntry)
            case namedEventEnabled(NamedEventEnabledEntry)
            
            case setDeadline(SetDeadlineEntry)
            case passedDeadline(PassedDeadlineEntry)
            
            case compareFailed(CompareFailedEntry)
        }
        
        public struct BeginTraceEntry {
            public var graph: Graph
        }
        
        public struct EndTraceEntry {
            public var graph: Graph
        }
        
        public struct BeginSubgraphUpdateEntry {
            public var subgraph: Subgraph
            public var flags: Subgraph.Flags
        }
        
        public struct EndSubgraphUpdateEntry {
            public var subgraph: Subgraph
        }
        
        public struct BeginNodeUpdateEntry {
            public var attribute: AnyAttribute
        }
        
        public struct EndNodeUpdateEntry {
            public var attribute: AnyAttribute
            public var changed: Bool
        }
        
        public struct BeginValueUpdateEntry {
            public var attribute: AnyAttribute
        }
        
        public struct EndValueUpdateEntry {
            public var attribute: AnyAttribute
            public var changed: Bool
        }
        
        public struct BeginGraphUpdateEntry {
            public var graph: Graph
        }
        
        public struct EndGraphUpdateEntry {
            public var graph: Graph
        }
        
        public struct BeginGraphInvalidationEntry {
            public var graph: Graph
            public var attribute: AnyAttribute
        }
        
        public struct EndGraphInvalidationEntry {
            public var graph: Graph
            public var attribute: AnyAttribute
        }
        
        public struct BeginModifyNodeEntry {
            public var attribute: AnyAttribute
        }
        
        public struct EndModifyNodeEntry {
            public var attribute: AnyAttribute
        }
        
        public struct BeginEventEntry {
            public var attribute: AnyAttribute
            public var eventName: String
        }
        
        public struct EndEventEntry {
            public var attribute: AnyAttribute
            public var eventName: String
        }
        
        public struct GraphCreatedEntry {
            public var graph: Graph
        }
        
        public struct GraphDestroyEntry {
            public var graph: Graph
        }
        
        public struct GraphNeedsUpdateEntry {
            public var graph: Graph
        }
        
        public struct SubgraphCreatedEntry {
            public var subgraph: Subgraph
        }

        public struct SubgraphDestroyEntry {
            public var subgraph: Subgraph
        }

        public struct SubgraphAddChildEntry {
            public var subgraph: Subgraph
            public var childSubgraph: Subgraph
        }
        
        public struct SubgraphRemoveChildEntry {
            public var subgraph: Subgraph
            public var childSubgraph: Subgraph
        }
        
        public struct NodeAddedEntry {
            public var attribute: AnyAttribute
        }
        
        public struct NodeAddEdgeEntry {
            public var attribute: AnyAttribute
            public var inputAttribute: AnyAttribute
            public var options: InputOptions
        }
        
        public struct NodeRemoveEdgeEntry {
            public var attribute: AnyAttribute
            public var inputIndex: Int
        }
        
        public struct NodeSetEdgePendingEntry {
            public var attribute: AnyAttribute
            public var inputIndex: Int
            public var pending: Bool
        }
        
        public struct NodeSetDirtyEntry {
            public var attribute: AnyAttribute
            public var dirty: Bool
        }
        
        public struct NodeSetPendingEntry {
            public var attribute: AnyAttribute
            public var pending: Bool
        }
        
        public struct NodeSetValueEntry {
            public var attribute: AnyAttribute
            public var value: UnsafeRawPointer
        }
        
        public struct NodeMarkValueEntry {
            public var attribute: AnyAttribute
        }
        
        public struct IndirectNodeAddedEntry {
            public var attribute: AnyAttribute
        }
        
        public struct IndirectNodeSetSourceEntry {
            public var attribute: AnyAttribute
            public var source: AnyAttribute
        }
        
        public struct IndirectNodeSetDependencyEntry {
            public var attribute: AnyAttribute
            public var dependency: AnyAttribute
        }
        
        public struct ProfileMarkEntry {
            public var eventName: String
        }
        
        public struct CustomEventEntry {
            public var graph: Graph
            public var eventName: String
            public var value: UnsafeRawPointer
            public var type: Any.Type
        }
        
        public struct NamedEventEntry {
            public var graph: Graph
            public var eventID: Graph.NamedTraceEventID
            public var eventArgs: [UInt32]
            public var data: Data?
            public var flags: Graph.NamedTraceEventFlags
        }
        
        public struct NamedEventEnabledEntry {
            public var eventID: Graph.NamedTraceEventID
        }
        
        public struct SetDeadlineEntry {
            public var deadline: UInt
        }
        
        public struct PassedDeadlineEntry {
        }
        
        public struct CompareFailedEntry {
            public var attribute: AnyAttribute
            public var comparisonState: ComparisonState
        }
    }
}

extension TestTraceRecorder.History {
    public var subgraphCreatedEntries: [TestTraceRecorder.History.SubgraphCreatedEntry] {
        entries.compactMap { entry in
            if case .subgraphCreated(let subgraphCreatedEntry) = entry {
                subgraphCreatedEntry
            } else {
                nil
            }
        }
    }
    
    public var subgraphDestroyEntries: [TestTraceRecorder.History.SubgraphDestroyEntry] {
        entries.compactMap { entry in
            if case .subgraphDestroy(let subgraphDestroyEntry) = entry {
                subgraphDestroyEntry
            } else {
                nil
            }
        }
    }
    
    public var subgraphAddChildEntries: [TestTraceRecorder.History.SubgraphAddChildEntry] {
        entries.compactMap { entry in
            if case .subgraphAddChild(let subgraphAddChildEntry) = entry {
                subgraphAddChildEntry
            } else {
                nil
            }
        }
    }
    
    public var subgraphRemoveChildEntries: [TestTraceRecorder.History.SubgraphRemoveChildEntry] {
        entries.compactMap { entry in
            if case .subgraphRemoveChild(let subgraphRemoveChildEntry) = entry {
                subgraphRemoveChildEntry
            } else {
                nil
            }
        }
    }
}
