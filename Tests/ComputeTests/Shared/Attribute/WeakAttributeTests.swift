import Testing

@Suite
struct WeakAttributeTests {
    
    @MainActor
    @Suite(.applySubgraph)
    struct InitTests {
        
        @Test
        func initDefault() {
            let weakAttribute = WeakAttribute<Int>()
            #expect(weakAttribute.attribute == nil)
        }
        
        @Test
        func initWithNil() {
            let weakAttribute = WeakAttribute<Int>(nil)
            #expect(weakAttribute.attribute == nil)
        }
        
        @Test
        func initWithAttribute() {
            let attribute = Attribute(value: 0)
            let weakAttribute = WeakAttribute(attribute)
            #expect(weakAttribute.attribute == attribute)
        }
        
    }
    
    @Suite
    struct SubgraphTraversalTests {

        @Test
        func invalidatingSubgraphNilsWeakAttribute() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            Subgraph.current = subgraph
            
            let attribute = Attribute(value: 0)
            let weakAttribute = WeakAttribute(attribute)
            #expect(weakAttribute.attribute == attribute)
            
            subgraph.invalidate()
            
            #expect(weakAttribute.attribute == nil)
            
            Subgraph.current = nil
        }

    }

}
