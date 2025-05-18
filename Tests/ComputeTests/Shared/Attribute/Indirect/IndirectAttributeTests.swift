import Testing

@Suite
final class IndirectAttributeTests {

    nonisolated(unsafe) private static let sharedGraph = Graph()
    private var graph: Graph
    private var subgraph: Subgraph

    init() {
        graph = Graph(shared: Self.sharedGraph)
        subgraph = Subgraph(graph: graph)
        Subgraph.current = subgraph
    }

    deinit {
        Subgraph.current = nil
    }

    @Test
    func basic() {
        let source = Attribute(value: 0)
        let indirect = IndirectAttribute(source: source)
        #expect(indirect.identifier != source.identifier)
        #expect(indirect.source.identifier == source.identifier)
        #expect(indirect.dependency == .init(rawValue: 0))
    }
}
