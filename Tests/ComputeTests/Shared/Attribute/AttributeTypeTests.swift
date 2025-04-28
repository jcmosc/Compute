import Foundation
import Testing

private struct CustomStruct {
    var property: Int
}

private enum CustomEnum {
    case first
    case second(Int)
    case third(String)
}

private indirect enum CustomRecursiveEnum {
    case none
    case recursive(CustomRecursiveEnum)
}

private struct CustomType {
    var string: String?
    var integer: Int?
    var structProperty: CustomStruct?
    var existential: (any Equatable)?
    var function: (() -> Void)?
    var enumProperty: CustomEnum?
    var recursiveEnumProperty: CustomRecursiveEnum?
}

@Suite
final class AttributeTypeTests {

    static let prefetchLayouts =
        Int(ProcessInfo.processInfo.environment["AG_PREFETCH_LAYOUTS", default: "0"], radix: 10) != 0
    static let asyncLayouts = Int(ProcessInfo.processInfo.environment["AG_ASYNC_LAYOUTS", default: "1"], radix: 10) != 0

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

    // TODO: use setenv
    @Test(.enabled(if: prefetchLayouts && !asyncLayouts))
    func layout() {
        let attribute = Attribute(
            value: CustomType(
                string: nil,
                integer: nil,
                structProperty: nil,
                existential: nil,
                function: nil,
                enumProperty: nil,
                recursiveEnumProperty: nil
            )
        )
        let attributeType = attribute.identifier.info.type.pointee

        #expect(attributeType.typeID == Metadata(External<CustomType>.self))
        #expect(attributeType.valueTypeID == Metadata(CustomType.self))
        #expect(attributeType.flags == AGAttributeTypeFlags(rawValue: 19))

        #expect(attributeType.layout != nil)
        if let layout = attributeType.layout {
            #expect(String(cString: layout) == "\u{1}\u{fffd}\u{fffd}D\u{4}\u{2}")
        }

    }

}
