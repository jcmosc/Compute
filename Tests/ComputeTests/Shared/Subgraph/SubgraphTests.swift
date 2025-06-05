import Foundation
import Testing

@Suite
struct SubgraphTests {

    @Suite
    struct CFTypeTests {

        @Test
        func typeID() {
            let description = CFCopyTypeIDDescription(Subgraph.typeID) as String?
            #expect(description == "AGSubgraph")
        }

        @Test
        func createSubgraph() async throws {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(CFGetTypeID(subgraph) == Subgraph.typeID)
        }

    }

    @Suite
    struct LifecycleTests {

        @Test
        func subgraphAddedToGraph() {
            let graph = Graph()

            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.graph == graph)
        }

        @Test
        func subgraphCounters() {
            let graph = Graph()

            #expect(graph.counter(for: .subgraphCount) == 0)
            #expect(graph.counter(for: .subgraphTotalCount) == 0)

            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                #expect(subgraph.graph == graph)

                #expect(graph.counter(for: .subgraphCount) == 1)
                #expect(graph.counter(for: .subgraphTotalCount) == 1)
            }

            #expect(graph.counter(for: .subgraphCount) == 0)
            #expect(graph.counter(for: .subgraphTotalCount) == 1)
        }

    }

    @Suite
    struct CurrentSubgraph {

        @Test
        func currentSubgraph() {
            #expect(Subgraph.current == nil)

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            Subgraph.current = subgraph
            #expect(Subgraph.current == subgraph)

            Subgraph.current = nil
            #expect(Subgraph.current == nil)
        }

    }

}
