import Testing

@Suite
struct RuleTests {

    @MainActor
    @Suite(.applySubgraph)
    struct ValueTests {

        struct TestRule: Rule {
            var value: String {
                return "computed"
            }
        }

        @Test
        func value() {
            let attribute = Attribute(TestRule())
            let value = attribute.value
            #expect(value == "computed")
        }

        @Test
        func initialValue() {
            let attribute = Attribute(TestRule(), initialValue: "initial")
            let value = attribute.value
            #expect(value == "initial")
        }

        @Test
        func invalidateValue() {
            let attribute = Attribute(TestRule(), initialValue: "initial")
            attribute.invalidateValue()
            let value = attribute.value
            #expect(value == "computed")
        }

    }

    @MainActor
    @Suite(.applySubgraph)
    struct InputTests {
        struct TestRule1: Rule {
            var value: String {
                return "rule 1 computed value"
            }
        }
        struct TestRule2: Rule {
            @Attribute var property: String
            var value: String {
                return "derived: \(property)"
            }
        }

        @Test
        func value() {
            let attribute1 = Attribute(TestRule1())
            let attribute2 = Attribute(TestRule2(property: attribute1))
            let value = attribute2.value
            #expect(value == "derived: rule 1 computed value")
        }
        
        @Test
        func initialValue() {
            let attribute1 = Attribute(TestRule1(), initialValue: "rule 1 initial value")
            let attribute2 = Attribute(TestRule2(property: attribute1))
            let value = attribute2.value
            #expect(value == "derived: rule 1 initial value")
        }
        
        @Test
        func invalidateValue() {
            let attribute1 = Attribute(TestRule1(), initialValue: "rule 1 initial value")
            let attribute2 = Attribute(TestRule2(property: attribute1))
            attribute1.invalidateValue()
            let value = attribute2.value
            #expect(value == "derived: rule 1 computed value")
        }

        
    }

}
