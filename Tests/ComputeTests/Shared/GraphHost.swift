class GraphHost {
    
    private(set) var graph: Graph
    private(set) var subgraph: Subgraph
    
    init() {
        graph = Graph()
        subgraph = Subgraph(graph: graph)
        Subgraph.current = subgraph
    }
    
    deinit {
        Subgraph.current = nil
    }
    
}
