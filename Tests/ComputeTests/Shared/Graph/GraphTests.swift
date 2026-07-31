import Foundation
import Testing
import _ComputeTestSupport

@Suite(.serialized(for: \Subgraph.Type.current))
struct GraphTests {
    @Suite
    struct CFTypeTests {
        @Test
        func typeID() {
            #if os(Linux)
            let description = String(cfString: CFCopyTypeIDDescription(Graph.typeID))
            #else
            let description = CFCopyTypeIDDescription(Graph.typeID) as String?
            #endif
            #if COMPATIBILITY_TESTS
            #expect(description == "AGGraphStorage")
            #else
            #expect(description == "IAGGraphStorage")
            #endif
        }

        @Test
        func createGraph() async throws {
            let graph = Graph()
            #expect(CFGetTypeID(graph) == Graph.typeID)
        }
    }

    @Suite
    struct LifecycleTests {
        @Test
        func createGraph() {
            let graph = Graph()

            let graphID = graph.counter(for: .graphID)
            #expect(graphID != 0)

            let contextID = graph.counter(for: .contextID)
            #expect(contextID != 0)

            #expect(graphID != contextID)
        }

        @Test
        func createSharedGraph() async throws {
            let firstGraph = Graph(shared: nil)
            let secondGraph = Graph(shared: firstGraph)

            #expect(firstGraph.counter(for: .graphID) == secondGraph.counter(for: .graphID))
            #expect(firstGraph.counter(for: .contextID) != secondGraph.counter(for: .contextID))
        }
    }

    @Suite
    struct ContextTests {
        @Test
        func storesContextPointer() {
            let graph = Graph()
            #expect(graph.context == nil)

            withUnsafePointer(to: "Value") { pointer in
                graph.context = UnsafeRawPointer(pointer)
                #expect(graph.context == UnsafeRawPointer(pointer))
            }
        }
    }

    @Suite
    struct InternAttributeTypeTests {
        nonisolated(unsafe) static var testVtable = _AttributeVTable()

        init() {
            InternAttributeTypeTests.testVtable.type_destroy = { (pointer: UnsafeMutablePointer<_AttributeType>) in
                pointer.deallocate()
            }
        }

        @Test
        func internAttributeTypeAssignsIndex() {
            let graph = Graph()

            // First type index is not 0
            let intTypeIndex = internAttributeType(
                ctx: graph.graphContext,
                body: Metadata(External<Int>.self),
                makeAttributeType: {
                    let pointer = UnsafeMutablePointer<_AttributeType>.allocate(capacity: 1)
                    pointer.pointee.self_id = Metadata(External<Int>.self)
                    pointer.pointee.value_id = Metadata(Int.self)
                    withUnsafePointer(to: &InternAttributeTypeTests.testVtable) { testVtablePointer in
                        pointer.pointee.vtable = testVtablePointer
                    }
                    return UnsafePointer(pointer)
                }
            )
            #expect(intTypeIndex == 1)

            // A new type is assigned a new index
            let stringTypeIndex = internAttributeType(
                ctx: graph.graphContext,
                body: Metadata(External<String>.self),
                makeAttributeType: {
                    let pointer = UnsafeMutablePointer<_AttributeType>.allocate(capacity: 1)
                    pointer.pointee.self_id = Metadata(External<String>.self)
                    pointer.pointee.value_id = Metadata(String.self)
                    withUnsafePointer(to: &InternAttributeTypeTests.testVtable) { testVtablePointer in
                        pointer.pointee.vtable = testVtablePointer
                    }
                    return UnsafePointer(pointer)
                }
            )
            #expect(stringTypeIndex == 2)

            // Interning the same type reuses the same index
            let cachedIntTypeIndex = internAttributeType(
                ctx: graph.graphContext,
                body: Metadata(External<Int>.self),
                makeAttributeType: {
                    let pointer = UnsafeMutablePointer<_AttributeType>.allocate(capacity: 1)
                    pointer.pointee.self_id = Metadata(External<Int>.self)
                    pointer.pointee.value_id = Metadata(Int.self)
                    withUnsafePointer(to: &InternAttributeTypeTests.testVtable) { testVtablePointer in
                        pointer.pointee.vtable = testVtablePointer
                    }
                    return UnsafePointer(pointer)
                }
            )
            #expect(cachedIntTypeIndex == intTypeIndex)
        }

        nonisolated(unsafe) static var internedAttributeType: UnsafeMutablePointer<_AttributeType>? = nil

        @Test
        func internAttributeTypeInitializesSelfOffsetAndLayout() async throws {
            try await #require(processExitsWith: .success) {
                setenv(prefetchLayoutsEnvironmentVariable, "1", 1)
                setenv(asyncLayoutsEnvironmentVariable, "0", 1)

                let graph = Graph()

                let _ = internAttributeType(
                    ctx: graph.graphContext,
                    body: Metadata(External<Int>.self),
                    makeAttributeType: {
                        let pointer = UnsafeMutablePointer<_AttributeType>.allocate(capacity: 1)
                        pointer.pointee.self_id = Metadata(External<Int>.self)
                        pointer.pointee.value_id = Metadata(Int.self)

                        let vtablePointer = UnsafeMutablePointer<_AttributeVTable>.allocate(capacity: 1)
                        vtablePointer.pointee.type_destroy = { (pointer: UnsafeMutablePointer<_AttributeType>) in
                            pointer.deallocate()
                        }
                        pointer.pointee.vtable = UnsafePointer(vtablePointer)

                        GraphTests.InternAttributeTypeTests.internedAttributeType = pointer

                        return UnsafePointer(pointer)
                    }
                )

                let attributeType = GraphTests.InternAttributeTypeTests.internedAttributeType?.pointee
                #expect(attributeType?.self_id == Metadata(External<Int>.self))
                #expect(attributeType?.value_id == Metadata(Int.self))
                #expect(attributeType?.value_layout == UnsafePointer(bitPattern: 1))
                #expect(attributeType?.internal_offset == 28)  // size of Node rounded up to alignment of External<Int>
            }
        }
    }
}
