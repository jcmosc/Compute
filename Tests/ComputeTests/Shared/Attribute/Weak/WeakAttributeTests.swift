import Testing

@Suite
final class WeakAttributeTests {

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
    func initTest() {
        let _ = WeakAttribute<Int>()
        let _ = WeakAttribute<Int>(nil)
        let attr = Attribute(value: 0)
        let _ = WeakAttribute(attr)
    }

}
