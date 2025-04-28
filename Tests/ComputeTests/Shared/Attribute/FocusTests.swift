import Testing

struct Demo {
    var a: Int
    var b: Double
}

@Suite
final class FocusTests {

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
        let root = Attribute<Demo>(value: Demo(a: 0, b: 1.0))
        let type = Focus<Demo, Int>.self
        let focus = type.init(root: root, keyPath: \.a)
        let d = focus.description
        #expect(d == "• Int")
        #expect(focus.value == 0)
        #expect(type.flags == [])
    }

}
