import Testing

@Suite
struct GraphTests {

    @Suite
    struct CFTypeTests {
        
        @Test
        func typeID() {
            let description = CFCopyTypeIDDescription(Graph.typeID) as String?
            #expect(description == "AGGraphStorage")
        }
        
        @Test
        func createGraph() async throws {
            let graph = Graph()
            #expect(CFGetTypeID(graph) == Graph.typeID)
        }
        
    }

}
