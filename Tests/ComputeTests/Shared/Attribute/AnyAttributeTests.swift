import Testing

@Suite
struct AnyAttributeTests {
    @Suite(.serialized(for: \Subgraph.Type.current))
    struct InitTests {
        @Test
        func nilAttribute() {
            withGraph {
                #expect(AnyAttribute.nil.rawValue == 2)
            }
        }

        @Test
        func initWithAttribute() {
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyAttribute = AnyAttribute(attribute)

                #expect(anyAttribute == attribute.identifier)
            }
        }
        
        @Test
        func hashableConformance() {
            func hashValue<T>(of value: T) -> Int where T: Hashable {
                value.hashValue
            }
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyAttribute = AnyAttribute(attribute)

                #expect(hashValue(of: anyAttribute) == anyAttribute.hashValue)
            }
        }
        
        @Test
        func customStringConvertibleConformance() {
            func description<T>(of value: T) -> String where T: CustomStringConvertible {
                value.description
            }
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyAttribute = AnyAttribute(attribute)

                #expect(anyAttribute.description == "#\(anyAttribute.rawValue)")
                #expect(description(of: anyAttribute) == attribute.description)
            }
        }
    }
}
