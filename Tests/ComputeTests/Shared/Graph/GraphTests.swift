import Foundation
import Testing

@Suite
struct GraphTests {

    @Suite
    struct CFTypeTests {

        @Test
        func typeID() {
            let description = CFCopyTypeIDDescription(Graph.typeID) as String?
            #expect(description == "AGGraphStorage")
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

        nonisolated(unsafe) static var testCallbacks = AGAttributeVTable()

        init() {
            InternAttributeTypeTests.testCallbacks.deallocate = { (pointer: UnsafeMutablePointer<AGAttributeType>) in
                pointer.deallocate()
            }
        }

        @Test
        func internAttributeTypeAssignsIndex() {
            let graph = Graph()

            // First type index is not 0
            let intTypeIndex = __AGGraphInternAttributeType(
                graph.graphContext,
                Metadata(External<Int>.self),
                { _ in
                    var attributeType = AGAttributeType()
                    attributeType.selfType = Metadata(External<Int>.self)
                    attributeType.valueType = Metadata(Int.self)

                    withUnsafePointer(to: &InternAttributeTypeTests.testCallbacks) { testCallbacksPointer in
                        attributeType.callbacks = testCallbacksPointer
                    }

                    let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
                    pointer.pointee = attributeType
                    return UnsafePointer(pointer)
                },
                nil
            )
            #expect(intTypeIndex == 1)

            // A new type is assigned a new index
            let stringTypeIndex = __AGGraphInternAttributeType(
                graph.graphContext,
                Metadata(External<String>.self),
                { _ in
                    var attributeType = AGAttributeType()
                    attributeType.selfType = Metadata(External<String>.self)
                    attributeType.valueType = Metadata(String.self)

                    withUnsafePointer(to: &InternAttributeTypeTests.testCallbacks) { testCallbacksPointer in
                        attributeType.callbacks = testCallbacksPointer
                    }

                    let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
                    pointer.pointee = attributeType
                    return UnsafePointer(pointer)
                },
                nil
            )
            #expect(stringTypeIndex == 2)

            // Interning the same type reuses the same index
            let cachedIntTypeIndex = __AGGraphInternAttributeType(
                graph.graphContext,
                Metadata(External<Int>.self),
                { _ in
                    var attributeType = AGAttributeType()
                    attributeType.selfType = Metadata(External<Int>.self)
                    attributeType.valueType = Metadata(Int.self)

                    withUnsafePointer(to: &InternAttributeTypeTests.testCallbacks) { testCallbacksPointer in
                        attributeType.callbacks = testCallbacksPointer
                    }

                    let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
                    pointer.pointee = attributeType
                    return UnsafePointer(pointer)
                },
                nil
            )
            #expect(cachedIntTypeIndex == intTypeIndex)
        }

        nonisolated(unsafe) static var internedAttributeType: UnsafeMutablePointer<AGAttributeType>? = nil

        @Test
        func internAttributeTypeInitializesSelfOffsetAndLayout() throws {
            try #require(ProcessInfo.processInfo.environment["AG_PREFETCH_LAYOUTS"] == "1")

            let graph = Graph()

            let _ = __AGGraphInternAttributeType(
                graph.graphContext,
                Metadata(External<Int>.self),
                { _ in
                    var attributeType = AGAttributeType()
                    attributeType.selfType = Metadata(External<Int>.self)
                    attributeType.valueType = Metadata(Int.self)

                    withUnsafePointer(to: &InternAttributeTypeTests.testCallbacks) { testCallbacksPointer in
                        attributeType.callbacks = testCallbacksPointer
                    }

                    let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
                    pointer.pointee = attributeType

                    GraphTests.InternAttributeTypeTests.internedAttributeType = pointer

                    return UnsafePointer(pointer)
                },
                nil
            )

            let attributeType = GraphTests.InternAttributeTypeTests.internedAttributeType?.pointee
            #expect(attributeType?.selfType == Metadata(External<Int>.self))
            #expect(attributeType?.valueType == Metadata(Int.self))
            #expect(attributeType?.layout == ValueLayout.trivial.storage)
            #expect(attributeType?.selfOffset == 28)  // size of Node rounded up to alignment of External<Int>
        }
    }

    @Suite
    struct DescriptionTests {

        @Test
        func initialDescription() {
            let description = Graph.description(nil, options: NSDictionary())
            #expect(description == nil)
        }

        @Test
        func graphDescription() {
            let graph = Graph()
            let description = Graph.description(graph, options: NSDictionary())
            #expect(description == nil)
        }

        @Suite
        struct GraphDictFormatTests {

            @Test
            func initialDescription() throws {
                let description =
                    try #require(
                        Graph.description(nil, options: [AGDescriptionFormat: "graph/dict"] as NSDictionary)
                            as? NSDictionary
                    )

                let json = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                #expect(
                    String(data: json, encoding: .utf8) == """
                        {
                          "graphs" : [

                          ],
                          "version" : 2
                        }
                        """
                )
            }

            @Test
            func graphDescription() throws {
                let graph = Graph()
                let description =
                    try #require(
                        Graph.description(graph, options: [AGDescriptionFormat: "graph/dict"] as NSDictionary)
                            as? NSDictionary
                    )

                let json = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                #expect(
                    String(data: json, encoding: .utf8) == """
                        {
                          "graphs" : [
                            {
                              "change_count" : 0,
                              "edges" : [

                              ],
                              "id" : \(graph.counter(for: .graphID)),
                              "nodes" : [

                              ],
                              "subgraphs" : [

                              ],
                              "transaction_count" : 0,
                              "types" : [

                              ],
                              "update_count" : 0
                            }
                          ],
                          "version" : 2
                        }
                        """
                )
            }
        }

    }

}
