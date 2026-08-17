import Testing

@Suite
struct SubgraphUpdateTests {
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
