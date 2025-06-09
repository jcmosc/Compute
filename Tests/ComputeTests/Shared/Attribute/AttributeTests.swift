import Testing

struct ProtocolConformance {
    var metadata: Metadata
    var witnessTable: UnsafeRawPointer
}

// This suite needs to run serialized to avoid creating duplicate layouts
@Suite(.serialized)
final class AttributeTests {

    private var graph: Graph
    private var subgraph: Subgraph

    init() {
        graph = Graph()
        subgraph = Subgraph(graph: graph)
        Subgraph.current = subgraph
    }

    deinit {
        Subgraph.current = nil
    }
    
    @Test
    func createAttributeWithValue() {
        let attribute = Attribute(value: 1)
        let attributeType = attribute.identifier.info.type.pointee
        
        let expectedlayout = prefetchCompareValues(
            of: Int.self,
            options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
            priority: 0
        )
        
        #expect(attributeType.selfType == Metadata(External<Int>.self))
        #expect(attributeType.valueType == Metadata(Int.self))
        
        #expect(attributeType.flags == [.external, .comparisonModeEquatableAlways])
        #expect(attributeType.selfOffset == 28)
        #expect(attributeType.layout.map { ValueLayout(storage: $0) } == expectedlayout)
        
        let conformance: ProtocolConformance = unsafeBitCast(
            External<Int>.self as any _AttributeBody.Type,
            to: ProtocolConformance.self
        )
        #expect(attributeType.attributeBodyType == conformance.metadata)
        #expect(attributeType.attributeBodyWitnessTable == conformance.witnessTable)
    }

    @Test
    func createAttributeWithType() {
        let attribute = Attribute(type: Int.self)
        let attributeType = attribute.identifier.info.type.pointee

        let expectedlayout = prefetchCompareValues(
            of: Int.self,
            options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
            priority: 0
        )

        #expect(attributeType.selfType == Metadata(External<Int>.self))
        #expect(attributeType.valueType == Metadata(Int.self))

        #expect(attributeType.flags == [.external, .comparisonModeEquatableAlways])
        #expect(attributeType.selfOffset == 28)
        #expect(attributeType.layout.map { ValueLayout(storage: $0) } == expectedlayout)

        let conformance: ProtocolConformance = unsafeBitCast(
            External<Int>.self as any _AttributeBody.Type,
            to: ProtocolConformance.self
        )
        #expect(attributeType.attributeBodyType == conformance.metadata)
        #expect(attributeType.attributeBodyWitnessTable == conformance.witnessTable)
    }

}
