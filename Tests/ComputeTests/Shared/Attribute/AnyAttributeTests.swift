import Testing

@Suite
struct AnyAttributeTests {
    @Suite(.serialized(for: \GraphHost.Type.sharedGraph))
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
    }
}
