import Testing

@Suite
struct SubgraphLifecycleTests {
    @Suite
    struct SubgraphCreatedTests {
        @Test
        func addedToGraph() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.graph == graph)
        }

        @Test
        func graphCountersIncremented() {
            let graph = Graph()

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 0)

            let subgraph = Subgraph(graph: graph)
            withExtendedLifetime(subgraph) {
                #expect(graph.counter(for: .subgraphs) == 1)
                #expect(graph.counter(for: .createdSubgraphs) == 1)
            }
        }
    }

    @Suite
    struct SubgraphInvalidateTests {
        @Test
        func removedFromGraph() async {
            await #expect(processExitsWith: .failure) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)

                subgraph.invalidate()

                print(subgraph.graph)  // will crash
            }
        }

        @Test
        func graphCountersDecrementedOnSubgraphInvalidate() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            #expect(graph.counter(for: .subgraphs) == 1)
            #expect(graph.counter(for: .createdSubgraphs) == 1)

            subgraph.invalidate()
            withExtendedLifetime(subgraph) {
                #expect(graph.counter(for: .subgraphs) == 0)
                #expect(graph.counter(for: .createdSubgraphs) == 1)
            }
        }
    }

    @Suite
    struct SubgraphDestroyTests {
        // This really just tests that Subgraph.deinit was triggered
        @Test
        func removedFromGraph() async {
            let graph = Graph()
            weak var weakSubgraph: Subgraph? = nil
            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                weakSubgraph = subgraph
            }
            #expect(weakSubgraph == nil)
        }

        @Test
        func graphCountersDecrementedOnSubgraphDeinit() {
            let graph = Graph()

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 0)

            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                withExtendedLifetime(subgraph) {
                    #expect(graph.counter(for: .subgraphs) == 1)
                    #expect(graph.counter(for: .createdSubgraphs) == 1)
                }
            }

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 1)
        }
    }
}
