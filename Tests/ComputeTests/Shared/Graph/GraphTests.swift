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
    struct TraceTests {

        @Test
        func addTrace() {
            class Context {
                var traceCalls: [(name: String, graph: Graph)] = []
            }

            var trace = AGTrace()
            trace.beginTrace = { contextPointer, graph in
                if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                    context.traceCalls.append((name: "beginTrace", graph: graph))
                }
            }
            trace.endTrace = { contextPointer, graph in
                if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                    context.traceCalls.append((name: "endTrace", graph: graph))
                }
            }

            let graph = Graph()
            var context = Context()

            #expect(graph.isTracingActive == false)

            let traceID = withUnsafeMutablePointer(to: &trace) { tracePointer in
                withUnsafeMutablePointer(to: &context) { contextPointer in
                    graph.addTrace(tracePointer, context: contextPointer)
                }
            }

            #expect(graph.isTracingActive == true)

            #expect(context.traceCalls.count == 1)
            #expect(context.traceCalls[0].name == "beginTrace")
            #expect(context.traceCalls[0].graph == graph)

            graph.removeTrace(traceID: traceID)

            #expect(context.traceCalls.count == 2)
            #expect(context.traceCalls[1].name == "endTrace")
            #expect(context.traceCalls[1].graph == graph)

            #expect(graph.isTracingActive == false)
        }

        @Test
        func setTrace() {
            class Context {
                var traceCalls: [(name: String, graph: Graph)] = []
            }

            var trace = AGTrace()
            trace.beginTrace = { contextPointer, graph in
                if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                    context.traceCalls.append((name: "beginTrace", graph: graph))
                }
            }
            trace.endTrace = { contextPointer, graph in
                if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                    context.traceCalls.append((name: "endTrace", graph: graph))
                }
            }

            let graph = Graph()
            var context = Context()

            withUnsafeMutablePointer(to: &trace) { tracePointer in
                withUnsafeMutablePointer(to: &context) { contextPointer in
                    graph.setTrace(tracePointer, context: contextPointer)
                }
            }

            #expect(context.traceCalls.count == 1)
            #expect(context.traceCalls[0].name == "beginTrace")
            #expect(context.traceCalls[0].graph == graph)

            graph.resetTrace()

            #expect(context.traceCalls.count == 2)
            #expect(context.traceCalls[1].name == "endTrace")
            #expect(context.traceCalls[1].graph == graph)
        }

        @Test
        func namedEvents() throws {
            let eventName = Graph.traceEventName(for: 0)
            #expect(eventName == nil)

            let eventID = "testname".utf8CString.withUnsafeBufferPointer { namePointer in
                "testsubsystem".utf8CString.withUnsafeBufferPointer { subsystemPointer in
                    return Graph.registerNamedTraceEvent(
                        name: namePointer.baseAddress!,
                        subsystem: subsystemPointer.baseAddress!
                    )
                }
            }

            let name = try #require(Graph.traceEventName(for: eventID))
            #expect(String(utf8String: name) == "testname")

            let subsystem = try #require(Graph.traceEventSubsystem(for: eventID))
            #expect(String(utf8String: subsystem) == "testsubsystem")
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

            // FIXME:
            // This sometimes fails because the subgraphs vector is sorted by pointer address,
            // which we can't predict deterministically.
            @Test
            func graphDescription() throws {
                let graph = Graph()
                
                let subgraph = Subgraph(graph: graph)
                let child = Subgraph(graph: graph)
                subgraph.addChild(child, tag: 1)
                
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
                                {
                                  "children" : [
                                    1
                                  ],
                                  "context_id" : 2,
                                  "id" : 1
                                },
                                {
                                  "context_id" : 2,
                                  "id" : 2,
                                  "parents" : [
                                    0
                                  ]
                                }
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
        
        @Suite
        struct BodyDescriptionTests {
            
            @Test
            func customBodyDescription() throws {
                struct TestBody: _AttributeBody, CustomStringConvertible {
                    var description: String {
                        return "Custom Description"
                    }
                }
                
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                Subgraph.current = subgraph
                
                let _ = withUnsafePointer(to: TestBody()) { bodyPointer in
                    Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                        return { _, _ in }
                    }
                }
                
                let description =
                try #require(
                    Graph.description(graph, options: [AGDescriptionFormat: "graph/dict"] as NSDictionary)
                    as? NSDictionary
                )
                let data = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                
                let graphDescription = try JSONDecoder().decode(GraphDescription.self, from: data)
                #expect(graphDescription.graphs[0].nodes[0].desc == "Custom Description")
            }
            
            @Test
            func defaultBodyDescription() throws {
                struct TestBody: _AttributeBody {
                    
                }
                
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                Subgraph.current = subgraph
                
                let _ = withUnsafePointer(to: TestBody()) { bodyPointer in
                    Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                        return { _, _ in }
                    }
                }
                
                let description =
                try #require(
                    Graph.description(graph, options: [AGDescriptionFormat: "graph/dict"] as NSDictionary)
                    as? NSDictionary
                )
                let data = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                
                let graphDescription = try JSONDecoder().decode(GraphDescription.self, from: data)
                #expect(graphDescription.graphs[0].nodes[0].desc == "GraphTests.DescriptionTests.BodyDescriptionTests.TestBody")
            }
            
        }
        
        @Suite
        struct ValueDescriptionTests {
            
            @Test
            func customValueDescription() throws {
                struct TestValue: CustomStringConvertible {
                    var field: Int = 0
                    var description: String {
                        return "Custom Value"
                    }
                }
                
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                Subgraph.current = subgraph
                
                let _ = Attribute(value: TestValue())
                
                let description =
                try #require(
                    Graph.description(graph, options: [AGDescriptionFormat: "graph/dict", AGDescriptionIncludeValues: true] as NSDictionary)
                    as? NSDictionary
                )
                let data = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                
                let graphDescription = try JSONDecoder().decode(GraphDescription.self, from: data)
                #expect(graphDescription.graphs[0].nodes[0].value == "Custom Value")
            }
            
            @Test
            func defaultValueDescription() throws {
                struct TestValue {
                    var field: Int = 0
                }
                
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                Subgraph.current = subgraph
                
                let _ = Attribute(value: TestValue())
                
                let description =
                try #require(
                    Graph.description(graph, options: [AGDescriptionFormat: "graph/dict", AGDescriptionIncludeValues: true] as NSDictionary)
                    as? NSDictionary
                )
                let data = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                
                let graphDescription = try JSONDecoder().decode(GraphDescription.self, from: data)
                #expect(graphDescription.graphs[0].nodes[0].desc == "GraphTests.DescriptionTests.ValueDescriptionTests.TestValue")
            }
            
        }

    }

}
