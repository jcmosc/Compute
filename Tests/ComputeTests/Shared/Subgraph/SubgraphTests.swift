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
        func createSubgraph() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.graph == graph)
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
