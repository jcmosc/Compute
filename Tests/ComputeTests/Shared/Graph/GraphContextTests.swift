import Testing

@Suite
struct GraphContextTests {

    @Test
    func currentGraphContext() {
        let graph = Graph()

        let subgraph = Subgraph(graph: graph)
        Subgraph.current = subgraph

        let currentGraphContext = Subgraph.currentGraphContext
        #expect(currentGraphContext != nil)
        #expect(currentGraphContext == graph.graphContext)
    }

}
