import Testing

@Suite
final class AnyAttributeTests {

    nonisolated(unsafe) private static let sharedGraph = Graph()
    private var graph: Graph
    private var subgraph: Subgraph

    init() {
        graph = Graph(shared: Self.sharedGraph)
        subgraph = Subgraph(graph: graph)
        Subgraph.current = subgraph
    }

    deinit {
        Subgraph.current = nil
    }

    @Test
    func constantValue() throws {
        let attributeNil = AnyAttribute.nil
        #expect(attributeNil.rawValue == 2)
    }

    @Test
    func description() throws {
        let attribute = AnyAttribute(rawValue: 0)
        #expect(attribute.description == "#0")

        let attributeNil = AnyAttribute.nil
        #expect(attributeNil.description == "#2")
    }

    @Test
    func current() {
        #expect(AnyAttribute.current == nil)
    }

    @Test
    func setFlags() throws {
        let attribute = AnyAttribute(Attribute(value: 0))
        #expect(attribute.flags == [])

        // Test mask = []
        attribute.flags = []

        attribute.setFlags([.active], mask: [])
        #expect(attribute.flags == [])

        attribute.setFlags([.removable], mask: [])
        #expect(attribute.flags == [])

        attribute.setFlags([.active, .invalidatable], mask: [])
        #expect(attribute.flags == [])

        // Test mask
        attribute.flags = []
        attribute.setFlags([.active], mask: [.active])
        #expect(attribute.flags == [.active])

        attribute.setFlags([.removable], mask: [.removable])
        #expect(attribute.flags == [.active, .removable])

        attribute.setFlags([.invalidatable], mask: [.active])
        #expect(attribute.flags == [.removable])

        attribute.setFlags([.active, .invalidatable], mask: [.active, .removable, .invalidatable])
        #expect(attribute.flags == [.active, .invalidatable])
    }

    @Test
    func visitBody() async {
        let attribute = Attribute(value: "string value")

        struct Visitor: AttributeBodyVisitor {
            let confirm: Confirmation
            func visit<Body>(body: UnsafePointer<Body>) where Body: _AttributeBody {
                if body.pointee is External<String> {
                    confirm()
                }
            }
        }

        await confirmation(expectedCount: 1) { confirm in
            var visitor = Visitor(confirm: confirm)
            attribute.visitBody(&visitor)
        }
    }

}
