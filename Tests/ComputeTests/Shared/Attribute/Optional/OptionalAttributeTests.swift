import Testing

@Suite
final class OptionalAttributeTests {

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
    func basicInit() {
        let ao1 = AnyOptionalAttribute()
        let o1 = OptionalAttribute<Void>()
        #expect(o1 == OptionalAttribute<Void>(base: ao1))

        let attr = Attribute<Void>(identifier: .init(rawValue: 0x1))
        let ao2 = AnyOptionalAttribute(attr.identifier)
        let o2 = OptionalAttribute(attr)
        #expect(o2 == OptionalAttribute<Void>(base: ao2))

        let o3 = OptionalAttribute<Void>(nil)
        #expect(o3.base.identifier == .nil)
    }

    @Test(.disabled("crash for invalid data offset"))
    func initWithWeak() {
        let attr = Attribute(value: 0)
        let weakAttr = WeakAttribute(attr)
        let _ = OptionalAttribute(weakAttr)
    }

    @Test
    func description() {
        let o1 = OptionalAttribute<Void>()
        #expect(o1.description == "nil")

        let attr = AnyAttribute(rawValue: 0x1)
        let o2 = OptionalAttribute(Attribute<Void>(identifier: attr))
        #expect(o2.description == "#1")
    }

}
