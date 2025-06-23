import Testing

@Suite
struct AnyAttributeTests {

    @Suite
    final class InitTests: GraphHost {

        @Test
        func nilAttribute() {
            #expect(AnyAttribute.nil.rawValue == 2)
        }

        @Test
        func initWithAttribute() {
            let attribute = Attribute<Int>(value: 1)
            let anyAttribute = AnyAttribute(attribute)

            #expect(anyAttribute == attribute.identifier)
        }

    }


}
