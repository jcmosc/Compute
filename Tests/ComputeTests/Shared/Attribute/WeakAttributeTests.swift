import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct WeakAttributeTests {
    @Suite
    struct InitTests {
        @Test
        func initDefault() {
            withGraph {
                let weakAttribute = WeakAttribute<Int>()
                #expect(weakAttribute.attribute == nil)
            }
        }

        @Test
        func initWithNil() {
            withGraph {
                let weakAttribute = WeakAttribute<Int>(nil)
                #expect(weakAttribute.attribute == nil)
            }
        }

        @Test
        func initWithAttribute() {
            withGraph {
                let attribute = Attribute(value: 0)
                let weakAttribute = WeakAttribute(attribute)
                #expect(weakAttribute.attribute == attribute)
            }
        }
    }

    @Suite
    struct SubgraphTraversalTests {
        @Test
        func invalidatingSubgraphNilsWeakAttribute() throws {
            try withGraph {
                let globalSubgraph = try #require(Subgraph.current)
                let subgraph = Subgraph(graph: globalSubgraph.graph)
                globalSubgraph.addChild(subgraph)

                subgraph.apply {
                    let attribute = Attribute(value: 0)
                    let weakAttribute = WeakAttribute(attribute)
                    #expect(weakAttribute.attribute == attribute)

                    subgraph.invalidate()

                    #expect(weakAttribute.attribute == nil)
                }
            }
        }
    }
}
