import Testing

@Suite
struct SubgraphUpdateTests {
    @Test
    func updatePreservesChildrenBeyondInlineCapacity() async {
        await #expect(processExitsWith: .success) {
            struct CountingRule: StatefulRule {
                typealias Value = Int
                var count = 0

                mutating func updateValue() {
                    count += 1
                    value = count
                }
            }

            withGraph {
                let parent = Subgraph.current!
                let flags = Subgraph.Flags(rawValue: 1)
                let children = (0..<64).map { _ in Subgraph(graph: parent.graph) }
                let attributes = children.map { child in
                    parent.addChild(child)
                    return child.apply {
                        let attribute = Attribute(CountingRule())
                        attribute.flags = flags
                        return attribute
                    }
                }

                for expectedValue in 1...3 {
                    for attribute in attributes {
                        attribute.invalidateValue()
                    }
                    parent.update(flags: flags)
                    for child in children {
                        precondition(child.isValid)
                    }
                    for attribute in attributes {
                        precondition(attribute.value == expectedValue)
                    }
                }
                withExtendedLifetime(children) {}
            }
        }
    }

    @Test
    func updateIterativelyEvaluatesDirtiedAttributesDuringTheSamePass() {
        class Counter {
            var ruleACount = 0
            var ruleBCount = 0
        }
        
        struct RuleA: Rule {
            let counter: Counter
            var value: Int {
                counter.ruleACount += 1
                return counter.ruleACount
            }
        }
        
        struct RuleB: Rule {
            let counter: Counter
            let sibling: Attribute<Int>
            var value: Int {
                counter.ruleBCount += 1
                if counter.ruleBCount > 1 {
                    sibling.invalidateValue()
                }
                return counter.ruleBCount
            }
        }
        
        withGraph {
            let subgraph = Subgraph.current!
            
            let counter = Counter()
            let attributeA = Attribute(RuleA(counter: counter))
            let attributeB = Attribute(RuleB(counter: counter, sibling: attributeA))

            attributeA.flags = Subgraph.Flags(rawValue: 1)
            attributeB.flags = Subgraph.Flags(rawValue: 1)

            #expect(attributeA.value == 1)
            #expect(attributeB.value == 1)
            #expect(attributeA.valueState.contains(.dirty) == false)
            #expect(attributeB.valueState.contains(.dirty) == false)

            attributeB.invalidateValue()
            
            // Only Attribute B should be dirty
            #expect(attributeA.valueState.contains(.dirty) == false)
            #expect(attributeB.valueState.contains(.dirty) == true)
            #expect(subgraph.isDirty(flags: Subgraph.Flags(rawValue: 1)) == true)
            
            subgraph.update(flags: Subgraph.Flags(rawValue: 1))
            
            // Both attributes should be re-evaluated in the same pass
            #expect(attributeA.value == 2)
            #expect(attributeB.value == 2)
            #expect(attributeA.valueState.contains(.dirty) == false)
            #expect(attributeB.valueState.contains(.dirty) == false)
            #expect(subgraph.isDirty(flags: Subgraph.Flags(rawValue: 1)) == false)
        }
    }
}
