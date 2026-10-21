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

    @Suite
    struct SubgraphBatchInvalidateTests {
        @Test
        func invalidateManySubgraphs() {
            let graph = Graph()

            var subgraphs: [Subgraph] = []
            for _ in 0..<10_000 {
                var parent = Subgraph(graph: graph)
                subgraphs.append(parent)
                for _ in 0..<3 {
                    let child = Subgraph(graph: graph)
                    parent.addChild(child)
                    subgraphs.append(child)
                    parent = child
                }
            }
            #expect(graph.counter(for: .subgraphs) == 40_000)

            graph.withoutSubgraphInvalidation {
                for subgraph in subgraphs {
                    subgraph.invalidate()
                }
            }
            #expect(graph.counter(for: .subgraphs) == 0)

            withExtendedLifetime(subgraphs) {}
        }

        #if canImport(Darwin)
        @Test(arguments: [1, 3, 5])
        func remainingSubgraphsAfterPartialInvalidate(keepEvery: Int) throws {
            let graph = Graph()

            let root = Subgraph(graph: graph)
            var subgraphs: [Subgraph] = []
            for _ in 0..<1_000 {
                let subgraph = Subgraph(graph: graph)
                root.addChild(subgraph)
                subgraphs.append(subgraph)
            }

            var invalidated: [Subgraph] = []
            var remaining: [Subgraph] = []
            for (index, subgraph) in subgraphs.enumerated() {
                if index % (keepEvery + 1) == 0 {
                    remaining.append(subgraph)
                } else {
                    invalidated.append(subgraph)
                }
            }
            graph.withoutSubgraphInvalidation {
                for subgraph in invalidated {
                    subgraph.invalidate()
                }
            }

            // New subgraphs may reuse the addresses of invalidated ones
            for _ in 0..<100 {
                let subgraph = Subgraph(graph: graph)
                root.addChild(subgraph)
                remaining.append(subgraph)
            }

            #expect(graph.counter(for: .subgraphs) == remaining.count + 1)
            #expect(root.childCount == remaining.count)

            let description = try #require(graph.dictionaryDescription())
            let graphDescription = try #require(description.graphs.first)
            #expect(graphDescription.subgraphs.count == remaining.count + 1)

            let rootIndex = try #require(graphDescription.subgraphs.firstIndex { $0.children != nil })
            let rootChildren = try #require(graphDescription.subgraphs[rootIndex].children)
            #expect(Set(rootChildren).count == remaining.count)
            for childIndex in rootChildren {
                #expect(graphDescription.subgraphs[childIndex].parents == [rootIndex])
            }

            withExtendedLifetime((root, subgraphs, remaining)) {}
        }
        #endif
    }
}
