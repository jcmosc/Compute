import Testing

struct Triple<A, B, C> {
    var first: A
    var second: B
    var third: C
}

extension Triple: Sendable where A: Sendable, B: Sendable, C: Sendable {}

// This suite needs to run serialized to avoid creating duplicate layouts
@Suite(.serialized)
struct AttributeTests {

    @Suite
    final class InitTests: GraphHost {

        @Test
        func initWithValue() {
            let attribute = Attribute(value: 1)
            #expect(attribute.value == 1)
            #expect(attribute.graph == graph)
            #expect(attribute.subgraph == subgraph)

            let expectedlayout = prefetchCompareValues(
                of: Int.self,
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            let attributeType = attribute.identifier.info.type.pointee
            #expect(attributeType.self_id == Metadata(External<Int>.self))
            #expect(attributeType.value_id == Metadata(Int.self))

            #expect(attributeType.flags == [.external, .comparisonModeEquatableAlways])
            #expect(attributeType.internal_offset == 28)
            #expect(attributeType.value_layout.map { ValueLayout(storage: $0) } == expectedlayout)

            let attributeBody = unsafeBitCast(
                External<Int>.self as any _AttributeBody.Type,
                to: (type: Metadata, witnessTable: UnsafeRawPointer).self
            )
            #expect(attributeType.body_conformance.type_id == attributeBody.type)
            #expect(attributeType.body_conformance.witness_table == attributeBody.witnessTable)
        }

        @Test
        func initWithType() {
            let attribute = Attribute(type: Int.self)
            #expect(attribute.graph == graph)
            #expect(attribute.subgraph == subgraph)

            let expectedlayout = prefetchCompareValues(
                of: Int.self,
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            let attributeType = attribute.identifier.info.type.pointee
            #expect(attributeType.self_id == Metadata(External<Int>.self))
            #expect(attributeType.value_id == Metadata(Int.self))

            #expect(attributeType.flags == [.external, .comparisonModeEquatableAlways])
            #expect(attributeType.internal_offset == 28)
            #expect(attributeType.value_layout.map { ValueLayout(storage: $0) } == expectedlayout)

            let attributeBody = unsafeBitCast(
                External<Int>.self as any _AttributeBody.Type,
                to: (type: Metadata, witnessTable: UnsafeRawPointer).self
            )
            #expect(attributeType.body_conformance.type_id == attributeBody.type)
            #expect(attributeType.body_conformance.witness_table == attributeBody.witnessTable)
        }

        @Test
        func initWithBody() {
            struct TestBody: _AttributeBody {

            }

            let attribute = withUnsafePointer(to: "test value") { valuePointer in
                withUnsafePointer(to: TestBody()) { bodyPointer in
                    Attribute(body: bodyPointer, value: valuePointer, flags: []) {
                        return { _, _ in

                        }
                    }
                }
            }
            #expect(attribute.value == "test value")
            #expect(attribute.graph == graph)
            #expect(attribute.subgraph == subgraph)

            let expectedlayout = prefetchCompareValues(
                of: String.self,
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            let attributeType = attribute.identifier.info.type.pointee
            #expect(attributeType.self_id == Metadata(TestBody.self))
            #expect(attributeType.value_id == Metadata(String.self))

            #expect(attributeType.flags == [.external, .comparisonModeEquatableAlways])
            #expect(attributeType.internal_offset == 28)
            #expect(attributeType.value_layout.map { ValueLayout(storage: $0) } == expectedlayout)

            let attributeBody = unsafeBitCast(
                External<Int>.self as any _AttributeBody.Type,
                to: (type: Metadata, witnessTable: UnsafeRawPointer).self
            )
            #expect(attributeType.body_conformance.type_id == attributeBody.type)
            #expect(attributeType.body_conformance.witness_table == attributeBody.witnessTable)
        }

        @Test
        func mainRef() {
            // if type flags is not async thread, value is not pod, and mainthraedonly set,
            // main_ref should be true
        }

        @Test
        func incrementsGraphCounters() {
            let attribute = Attribute(value: 1)

            #expect(attribute.graph.counter(for: .nodeCount) == 1)
            #expect(attribute.graph.counter(for: .nodeTotalCount) == 1)
        }

    }

    @Suite
    class BodyTests: GraphHost {

        @Test
        func visitBody() async {
            struct TestBody: _AttributeBody {
                var data: String
            }

            let attribute = withUnsafePointer(to: TestBody(data: "body data")) { bodyPointer in
                Attribute<TestBody>(body: bodyPointer, value: nil, flags: []) {
                    return { _, _ in }
                }
            }

            struct Visitor: AttributeBodyVisitor {
                var visited = false
                mutating func visit<Body: _AttributeBody>(body: UnsafePointer<Body>) {
                    guard let testBody = body.pointee as? TestBody else {
                        return
                    }
                    if testBody.data == "body data" {
                        visited = true
                    }
                }
            }

            var visitor = Visitor()
            attribute.visitBody(&visitor)
            #expect(visitor.visited == true)
        }

    }

    @Suite
    final class FlagsTests: GraphHost {

        @Test
        func initialFlags() {
            let attribute = AnyAttribute(Attribute(value: 0))
            #expect(attribute.flags == [])
        }

        @Test
        func unmasked() {
            let attribute = AnyAttribute(Attribute(value: 0))

            attribute.flags = []

            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: [])
            #expect(attribute.flags == [])

            attribute.setFlags(Subgraph.Flags(rawValue: 2), mask: [])
            #expect(attribute.flags == [])

            attribute.setFlags(Subgraph.Flags(rawValue: 5), mask: [])
            #expect(attribute.flags == [])
        }

        @Test
        func masked() {
            let attribute = AnyAttribute(Attribute(value: 0))

            attribute.flags = []
            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: [Subgraph.Flags(rawValue: 1)])
            #expect(attribute.flags == [Subgraph.Flags(rawValue: 1)])

            attribute.setFlags(Subgraph.Flags(rawValue: 2), mask: [Subgraph.Flags(rawValue: 2)])
            #expect(attribute.flags == Subgraph.Flags(rawValue: 3))

            attribute.setFlags(Subgraph.Flags(rawValue: 4), mask: [Subgraph.Flags(rawValue: 1)])
            #expect(attribute.flags == [Subgraph.Flags(rawValue: 2)])

            attribute.setFlags(Subgraph.Flags(rawValue: 5), mask: Subgraph.Flags(rawValue: 7))
            #expect(attribute.flags == Subgraph.Flags(rawValue: 5))
        }

    }

    @Suite
    final class OffsetTests: GraphHost {

        @Test
        func pointerOffset() {
            struct TestStruct {
                var a: Int
                var b: String
            }

            let attribute = Attribute(value: TestStruct(a: 1, b: "test data"))

            let offsetAttribute = attribute.applying(offset: PointerOffset<TestStruct, String>(byteOffset: 8))
            #expect(offsetAttribute.value == "test data")

            // Does not search offset attributes
            let foundAnyFromAttribute = attribute.breadthFirstSearch(options: [.searchInputs, .searchOutputs]) {
                candidate in
                if candidate == attribute.identifier {
                    return false  // skip self
                }
                return true
            }
            #expect(foundAnyFromAttribute == false)

            // resolves to attribute
            let foundAttribute = offsetAttribute.breadthFirstSearch(options: []) { candidate in
                if candidate == attribute.identifier {
                    return true
                }
                return false
            }
            #expect(foundAttribute == true)
        }

    }

    @Suite
    final class InputTests: GraphHost {

        @Test
        func addInput() {
            let attribute = Attribute(value: 0)
            let input = Attribute(value: 1)

            attribute.addInput(input, options: [], token: 0)

            var foundInput = attribute.breadthFirstSearch(options: []) { candidate in
                return candidate == input.identifier
            }
            #expect(foundInput == false)
            foundInput = attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == input.identifier
            }
            #expect(foundInput == true)

            var foundOutput = input.breadthFirstSearch(options: []) { candidate in
                return candidate == attribute.identifier
            }
            #expect(foundOutput == false)
            foundOutput = input.breadthFirstSearch(options: [.searchOutputs]) { candidate in
                return candidate == attribute.identifier
            }
            #expect(foundOutput == true)
        }

        @Test
        func addInputFromDifferentContext() {
            let attribute = Attribute(value: 0)

            let otherGraph = Graph(shared: graph)
            let otherSubgraph = Subgraph(graph: otherGraph)
            Subgraph.current = otherSubgraph

            let input = Attribute(value: 1)

            attribute.addInput(input, options: [], token: 0)

            var foundInput = attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == input.identifier
            }
            #expect(foundInput == false)
            foundInput = attribute.breadthFirstSearch(options: [.searchInputs, .traverseGraphContexts]) { candidate in
                return candidate == input.identifier
            }
            #expect(foundInput == true)

            var foundOutput = input.breadthFirstSearch(options: [.searchOutputs]) { candidate in
                return candidate == attribute.identifier
            }
            #expect(foundOutput == false)
            foundOutput = input.breadthFirstSearch(options: [.searchOutputs, .traverseGraphContexts]) { candidate in
                return candidate == attribute.identifier
            }
            #expect(foundOutput == true)
        }

    }

    @Suite
    final class ValueTests: GraphHost {

        @Test
        func validate() {
            let attribute = Attribute(value: 1)
            attribute.validate()
        }
        
        @Test
        func value() {
            let attribute = Attribute(value: 1)
            let value = attribute.value
            #expect(value == 1)
        }

    }

    @Suite
    final class SubscriptTests: GraphHost {

        struct TestStruct {
            var a: Int
            var b: String
        }

        @Test
        func offset() {
            let attribute = Attribute(value: TestStruct(a: 1, b: "b"))

            let member = attribute[offset: { base in
                return PointerOffset.of(&base.b)
            }]
            #expect(member.value == "b")
        }

    }

    @Suite
    final class DescriptionTests: GraphHost {

        @Test
        func description() throws {
            let zeroAttribute = AnyAttribute(rawValue: 0)
            #expect(zeroAttribute.description == "#0")

            let nilAttribute = AnyAttribute.nil
            #expect(nilAttribute.description == "#2")

            let valueAttribute = AnyAttribute(Attribute(value: 1))
            #expect(valueAttribute.description == "#\(valueAttribute.rawValue)")
        }

    }

    @Suite
    struct HashableTests {

        @Test
        func hashable() {
            let a = Attribute<Int>(identifier: .nil)
            let b = Attribute<Int>(identifier: .nil)
            #expect(a == b)
            #expect(a.hashValue == b.hashValue)
        }

    }

}
