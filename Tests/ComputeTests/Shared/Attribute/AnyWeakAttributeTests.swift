import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct AnyWeakAttributeTests {
    @Test
    func initWithAttribute() {
        withGraph {
            let attribute = Attribute<Int>(value: 1)
            let anyWeakAttribute = AnyWeakAttribute(attribute.identifier)

            #expect(anyWeakAttribute.attribute == attribute.identifier)
        }
    }
    
    @Test
    func initWithNil() {
        withGraph {
            let anyWeakAttribute = AnyWeakAttribute(nil)

            #expect(anyWeakAttribute.attribute == nil)
        }
    }
    
    @Test
    func hashableConformance() {
        func hashValue<T>(of value: T) -> Int where T: Hashable {
            value.hashValue
        }
        withGraph {
            let attribute = Attribute<Int>(value: 1)
            let anyWeakAttribute = AnyWeakAttribute(attribute.identifier)

            #expect(hashValue(of: anyWeakAttribute) == anyWeakAttribute.hashValue)
        }
    }
    
    @Test
    func customStringConvertibleConformance() {
        func description<T>(of value: T) -> String where T: CustomStringConvertible {
            value.description
        }
        withGraph {
            let attribute = Attribute<Int>(value: 1)
            let anyWeakAttribute = AnyWeakAttribute(attribute.identifier)

            #expect(anyWeakAttribute.description == "#\(attribute.identifier.rawValue)")
            #expect(description(of: anyWeakAttribute) == anyWeakAttribute.description)
        }
    }
}
