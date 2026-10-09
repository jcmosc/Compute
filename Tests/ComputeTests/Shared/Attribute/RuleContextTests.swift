import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct RuleContextTests {
    @Suite
    struct UpdateTests {
        @Test
        func anyRuleContextUpdateMakesAttributeCurrent() {
            withGraph {
                let attribute = Attribute(value: 1)

                var currentAttribute: AnyAttribute?
                var bodyCallCount = 0
                AnyRuleContext(attribute: attribute.identifier).update {
                    currentAttribute = AnyAttribute.current
                    bodyCallCount += 1
                }

                #expect(bodyCallCount == 1)
                #expect(currentAttribute == attribute.identifier)
                #expect(AnyAttribute.current == nil)
            }
        }

        @Test
        func ruleContextUpdateMakesAttributeCurrent() {
            withGraph {
                let attribute = Attribute(value: 1)

                var currentAttribute: AnyAttribute?
                var bodyCallCount = 0
                RuleContext(attribute: attribute).update {
                    currentAttribute = AnyAttribute.current
                    bodyCallCount += 1
                }

                #expect(bodyCallCount == 1)
                #expect(currentAttribute == attribute.identifier)
                #expect(AnyAttribute.current == nil)
            }
        }
    }
}
