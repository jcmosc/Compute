import Testing

@Suite
struct WeakAttributeTests {
    
    @Suite
    final class InitTests: GraphHost {
        
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
    final class SubgraphTraversalTests: GraphHost {

        @Test
        func invalidatingSubgraphNilsWeakAttribute() {
            let attribute = Attribute(value: 0)
            let weakAttribute = WeakAttribute(attribute)
            #expect(weakAttribute.attribute == attribute)
            
            subgraph.invalidate()
            
            #expect(weakAttribute.attribute == nil)
        }

    }

}
