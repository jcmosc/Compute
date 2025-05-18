import Testing

@Suite
final class ExternalTests {

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
    func example() throws {
        let type = External<Int>.self
        let externalInt = type.init()
        #expect(externalInt.description == "Int")
        #expect(type.comparisonMode == [._1, ._2])
        #expect(type.flags == [])
    }

}
