import Testing

@Suite
struct AnyOptionalAttributeTests {
    @Suite(.serialized(for: \Subgraph.Type.current))
    struct InitTests {
        @Test
        func initWithAnyAttribute() {
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyOptionalAttribute = AnyOptionalAttribute(attribute.identifier)

                #expect(anyOptionalAttribute.attribute == attribute.identifier)
            }
        }
        @Test
        func initWithNil() {
            withGraph {
                let anyOptionalAttribute = AnyOptionalAttribute(nil)

                #expect(anyOptionalAttribute.attribute == nil)
            }
        }
        
        @Test
        func hashableConformance() {
            func hashValue<T>(of value: T) -> Int where T: Hashable {
                value.hashValue
            }
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyOptionalAttribute = AnyOptionalAttribute(attribute.identifier)

                #expect(hashValue(of: anyOptionalAttribute) == anyOptionalAttribute.hashValue)
            }
        }
        
        @Test
        func customStringConvertibleConformance() {
            func description<T>(of value: T) -> String where T: CustomStringConvertible {
                value.description
            }
            withGraph {
                let attribute = Attribute<Int>(value: 1)
                let anyOptionalAttribute = AnyOptionalAttribute(attribute.identifier)

                #expect(anyOptionalAttribute.description == "#\(attribute.identifier.rawValue)")
                #expect(description(of: anyOptionalAttribute) == anyOptionalAttribute.description)
            }
        }
    }
}
