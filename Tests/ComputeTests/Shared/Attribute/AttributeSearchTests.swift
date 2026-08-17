import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct AttributeSearchTests {
    @Suite
    struct SearchResultTests {
        @Test
        func searchWithMatchReturnsTrue() {
            withGraph {
                let attribute = Attribute(value: 1)
                let input = Attribute(value: 2)
                attribute.addInput(input, options: [], token: 0)

                var visitCount = 0
                let found = attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                    visitCount += 1
                    return candidate == input.identifier
                }
                
                #expect(visitCount == 2)
                #expect(found == true)
            }
        }
        
        @Test
        func searchWithNoMatchReturnsFalse() {
            withGraph {
                let attribute = Attribute(value: 1)
                let input = Attribute(value: 2)
                attribute.addInput(input, options: [], token: 0)

                let unrelated = Attribute(value: 3)

                var visitCount = 0
                let found = attribute.breadthFirstSearch(options: [.searchInputs, .searchOutputs]) { candidate in
                    visitCount += 1
                    return candidate == unrelated.identifier
                }
                
                #expect(visitCount == 2)
                #expect(found == false)
            }
        }
        
        @Test
        func searchThatNeverMatchesReturnsFalse() {
            withGraph {
                let attribute = Attribute(value: 1)
                let input = Attribute(value: 2)
                attribute.addInput(input, options: [], token: 0)

                var visitCount = 0
                let found = attribute.breadthFirstSearch(options: [.searchInputs]) { _ in
                    visitCount += 1
                    return false
                }

                #expect(visitCount == 2)
                #expect(found == false)
            }
        }
        
        @Test
        func searchOfIsolatedAttributeReturnsFalse() {
            withGraph {
                let attribute = Attribute(value: 1)

                var visitCount = 0
                let found = attribute.breadthFirstSearch(options: [.searchInputs, .searchOutputs]) { _ in
                    visitCount += 1
                    return false
                }
                
                #expect(visitCount == 1)
                #expect(found == false)
            }
        }
    }
    
    @Suite
    struct AttributeResolutionTests {
        @Test
        func offsetAttributeInputIsResolvedBeforeVisiting() {
            struct Pair {
                var first: Int
                var second: Int
            }

            withGraph {
                let source = Attribute(value: Pair(first: 1, second: 2))
                let offset = source.unsafeOffset(at: MemoryLayout<Int>.size, as: Int.self)

                let destination = Attribute(value: 0)
                destination.addInput(offset, options: [], token: 0)

                var visited: [AnyAttribute] = []
                let found = destination.breadthFirstSearch(options: [.searchInputs]) { candidate in
                    visited.append(candidate)
                    return false
                }

                #expect(found == false)
                #expect(visited.contains(destination.identifier))
                #expect(visited.contains(source.identifier))
                #expect(!visited.contains(offset.identifier))
            }
        }
        
        @Test
        func indirectAttributeSourceIsResolvedBeforeVisiting() {
            withGraph {
                let source = Attribute(value: 1)
                let indirect = IndirectAttribute(source: source)

                let destination = Attribute(value: 0)
                destination.addInput(indirect.attribute, options: [], token: 0)

                var visited: [AnyAttribute] = []
                let found = destination.breadthFirstSearch(options: [.searchInputs]) { candidate in
                    visited.append(candidate)
                    return false
                }

                #expect(found == false)
                #expect(visited.contains(source.identifier))
                #expect(!visited.contains(indirect.identifier))
            }
        }
    }
    
    @Suite
    struct AttributeCycleTests {
        @Test
        func rootIsVisitedOnceWhenNoCycleExists() {
            withGraph {
                let a = Attribute(value: 1)
                let b = Attribute(value: 2)
                a.addInput(b, options: [], token: 0)

                var visited: [AnyAttribute] = []
                _ = a.breadthFirstSearch(options: [.searchInputs]) { candidate in
                    visited.append(candidate)
                    return false
                }

                #expect(visited == [a.identifier, b.identifier])
            }
        }
        
        @Test
        func rootIsVisitedOnceWhenCycleExists() {
            withGraph {
                let a = Attribute(value: 1)
                let b = Attribute(value: 2)

                // a <- b <- a
                a.addInput(b, options: [], token: 0)
                b.addInput(a, options: [], token: 0)

                var visited: [AnyAttribute] = []
                let found = a.breadthFirstSearch(options: [.searchInputs]) { candidate in
                    visited.append(candidate)
                    return false
                }

                #expect(found == false)
                #expect(visited == [a.identifier, b.identifier])
            }
        }
    }
}
