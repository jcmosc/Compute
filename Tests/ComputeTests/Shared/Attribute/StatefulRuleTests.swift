import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct StatefulRuleTests {
    @Suite
    struct ValueTests {
        struct TestRule: StatefulRule {
            typealias Value = String
            func updateValue() {
                value = "computed"
            }
        }

        @Test
        func value() {
            withGraph {
                let attribute = Attribute(TestRule())
                let value = attribute.value
                #expect(value == "computed")
            }
        }

        @Test
        func initialValue() {
            withGraph {
                let attribute = Attribute(TestRule(), initialValue: "initial")
                let value = attribute.value
                #expect(value == "initial")
            }
        }

        @Test
        func invalidateValue() {
            withGraph {
                let attribute = Attribute(TestRule(), initialValue: "initial")
                attribute.invalidateValue()
                let value = attribute.value
                #expect(value == "computed")
            }
        }
    }

    @Suite
    struct InputTests {
        struct TestRule1: StatefulRule {
            typealias Value = String
            func updateValue() {
                value = "rule 1 computed value"
            }
        }
        struct TestRule2: StatefulRule {
            typealias Value = String
            @Attribute var property: String
            func updateValue() {
                value = "derived: \(property)"
            }
        }

        @Test
        func value() {
            withGraph {
                let attribute1 = Attribute(TestRule1())
                let attribute2 = Attribute(TestRule2(property: attribute1))
                let value = attribute2.value
                #expect(value == "derived: rule 1 computed value")
            }
        }

        @Test
        func initialValue() {
            withGraph {
                let attribute1 = Attribute(TestRule1(), initialValue: "rule 1 initial value")
                let attribute2 = Attribute(TestRule2(property: attribute1))
                let value = attribute2.value
                #expect(value == "derived: rule 1 initial value")
            }
        }

        @Test
        func invalidateValue() {
            withGraph {
                let attribute1 = Attribute(TestRule1(), initialValue: "rule 1 initial value")
                let attribute2 = Attribute(TestRule2(property: attribute1))
                attribute1.invalidateValue()
                let value = attribute2.value
                #expect(value == "derived: rule 1 computed value")
            }
        }
    }
    
    @Suite
    struct ModifyTests {
        struct MutableRule: StatefulRule {
            typealias Value = String
            func updateValue() {
                value = "bodyChanged = \(bodyChanged)"
            }
        }
        
        @Test
        func modifyBody() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(MutableRule())
            }
            
            let result1 = attribute.value
            #expect(result1 == "bodyChanged = false")
            
            attribute.mutateBody(as: MutableRule.self, invalidating: false) { _ in }
            
            let result2 = attribute.value
            #expect(result2 == "bodyChanged = false")
        }
        
        @Test
        func modifyBodyInvalidating() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(MutableRule())
            }
            
            let result1 = attribute.value
            #expect(result1 == "bodyChanged = false")
            
            attribute.mutateBody(as: MutableRule.self, invalidating: true) { _ in }
            
            let result2 = attribute.value
            #expect(result2 == "bodyChanged = true")
        }
    }
}
