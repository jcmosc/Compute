class GraphHost {
    nonisolated(unsafe) static let sharedGraph: Graph = {
        let graph = Graph()
        return graph
    }()

    var graph: Graph?
    var globalSubgraph: Subgraph

    init() {
        let graph = Graph(shared: GraphHost.sharedGraph)
        let globalSubgraph = Subgraph(graph: graph)

        self.graph = graph
        self.globalSubgraph = globalSubgraph

        graph.context = UnsafeRawPointer(Unmanaged.passUnretained(self).toOpaque())
    }

    deinit {
        guard let graph else {
            return
        }
        globalSubgraph.invalidate()
        graph.context = nil
        graph.invalidate()
        self.graph = nil
    }
}

extension Graph {
    func graphHost() -> GraphHost {
        unsafeBitCast(context, to: GraphHost.self)
    }
}

func withGraph<T>(_ body: () throws -> T) rethrows -> T {
    let graphHost = GraphHost()
    let oldSubgraph = Subgraph.current
    Subgraph.current = graphHost.globalSubgraph
    defer { Subgraph.current = oldSubgraph }

    return try body()
}
