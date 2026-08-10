import Foundation
import Testing
import _ComputeTestSupport

#if canImport(Darwin)
extension Subgraph: @retroactive Comparable {
    // Subgraphs are sorted by pointer address
    public static func < (lhs: Subgraph, rhs: Subgraph) -> Bool {
        // Advance IAGSubgraphRef by sizeof(CFRuntimeBase) == 16 bytes to reach IAG::Subgraph *
        let lhsAddress = unsafeBitCast(lhs, to: UnsafePointer<UInt>.self).advanced(by: 2).pointee
        let rhsAddress = unsafeBitCast(rhs, to: UnsafePointer<UInt>.self).advanced(by: 2).pointee
        return lhsAddress < rhsAddress
    }
}

@Suite(.serialized(for: \Subgraph.Type.current))
struct GraphDescriptionTests {
    @Suite
    struct DefaultTests {
        @Test
        func emptyGraph() {
            let graph = Graph()

            let description = Graph.description(graph, options: NSDictionary())
            #expect(description == nil)
        }
    }

    @Suite
    struct DictionaryTests {
        @Test
        func emptyGraph() async throws {
            await #expect(processExitsWith: .success) {
                let graph = Graph()

                let description = graph.dictionaryDescription()
                let expectedDescription = Graph.DictionaryDescription(
                    version: 2,
                    counters: .init(bytes: 0, maxBytes: 0),
                    graphs: [
                        .init(
                            id: Int(graph.counter(for: .graphID)),
                            counters: .init(
                                nodes: 0,
                                createdNodes: 0,
                                maxNodes: 0,
                                subgraphs: 0,
                                createdSubgraphs: 0,
                                maxSubgraphs: 0,
                                updates: 0,
                                changes: 0,
                                transactions: 0
                            ),
                            types: [],
                            nodes: [],
                            edges: [],
                            subgraphs: [],
                            transactionCount: 0,
                            updateCount: 0,
                            changeCount: 0,
                        )
                    ]
                )

                assertValuesEqualWithDiff(description, expectedDescription)
            }
        }

        @Test
        func graphWithSubgraphs() async {
            await #expect(processExitsWith: .success) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let child = Subgraph(graph: graph)
                subgraph.addChild(child, tag: 1)

                let description = graph.dictionaryDescription()
                let expectedDescription = Graph.DictionaryDescription(
                    version: 2,
                    counters: .init(bytes: 0, maxBytes: 0),
                    graphs: [
                        .init(
                            id: Int(graph.counter(for: .graphID)),
                            counters: .init(
                                nodes: 0,
                                createdNodes: 0,
                                maxNodes: 0,
                                subgraphs: 2,
                                createdSubgraphs: 2,
                                maxSubgraphs: 2,
                                updates: 0,
                                changes: 0,
                                transactions: 0
                            ),
                            types: [],
                            nodes: [],
                            edges: [],
                            subgraphs: subgraph < child
                                ? [
                                    .init(id: 1, contextID: 2, children: [1]),
                                    .init(id: 2, contextID: 2, parents: [0]),
                                ]
                                : [
                                    .init(id: 2, contextID: 2, parents: [1]),
                                    .init(id: 1, contextID: 2, children: [0]),
                                ],
                            transactionCount: 0,
                            updateCount: 0,
                            changeCount: 0
                        )
                    ]
                )

                assertValuesEqualWithDiff(description, expectedDescription)
            }
        }

        @Test
        func customBodyDescription() throws {
            struct TestBody: _AttributeBody, CustomStringConvertible {
                var description: String {
                    return "Custom Description"
                }
            }

            try withGraph {
                let _ = withUnsafePointer(to: TestBody()) { bodyPointer in
                    Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                        return { _, _ in }
                    }
                }

                let graphDescription = try #require(Subgraph.current?.graph.dictionaryDescription())
                #expect(graphDescription.graphs[0].nodes[0].description == "Custom Description")
            }
        }

        @Test
        func defaultBodyDescription() throws {
            struct TestBody: _AttributeBody {

            }

            try withGraph {
                let _ = withUnsafePointer(to: TestBody()) { bodyPointer in
                    Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                        return { _, _ in }
                    }
                }

                let graphDescription = try #require(Subgraph.current?.graph.dictionaryDescription())
                #expect(
                    graphDescription.graphs[0].nodes[0].description
                        == "GraphDescriptionTests.DictionaryTests.TestBody"
                )
            }
        }

        @Test
        func customValueDescription() throws {
            struct TestValue: CustomStringConvertible {
                var field: Int = 0
                var description: String {
                    return "Custom Value"
                }
            }

            try withGraph {
                let _ = Attribute(value: TestValue())

                let graphDescription = try #require(
                    Subgraph.current?.graph.dictionaryDescription(includeValues: true)
                )
                #expect(graphDescription.graphs[0].nodes[0].value == "Custom Value")
            }
        }

        @Test
        func defaultValueDescription() throws {
            struct TestValue {
                var field: Int = 0
            }

            try withGraph {
                let _ = Attribute(value: TestValue())

                let graphDescription = try #require(
                    Subgraph.current?.graph.dictionaryDescription(includeValues: true)
                )
                #expect(
                    graphDescription.graphs[0].nodes[0].value
                        == "TestValue(field: 0)"
                )
            }
        }
    }

    @Suite
    struct GraphvizTests {
        struct TestRule: Rule {
            @Attribute var input: Int
            var value: Int { input + 1 }
        }

        @Test
        func graphWithNodes() async {
            await #expect(processExitsWith: .success) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                subgraph.apply {
                    let attribute1 = Attribute(value: 1)
                    let _ = Attribute(TestRule(input: attribute1))
                }

                let description = graph.graphvizDescription(includeValues: false)
                #expect(
                    description == """
                        digraph {
                          _576[label="576: GraphDescriptionTests.GraphvizTests.Test…" style="dashed" color=red];
                          _536[label="536: External value" style="bold"];
                        }

                        """
                )
            }
        }

        @Test
        func graphWithNodesUpdated() async {
            await #expect(processExitsWith: .success) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let attribute2 = subgraph.apply {
                    let attribute1 = Attribute(value: 1)
                    return Attribute(TestRule(input: attribute1))
                }

                let _ = attribute2.value

                let description = graph.graphvizDescription(includeValues: false)
                #expect(
                    description == """
                        digraph {
                          _576[label="576: GraphDescriptionTests.GraphvizTests.Test…" style="bold"];
                          _536 -> _576[];
                          _536[label="536: External value" style="bold"];
                        }

                        """
                )
            }
        }

        @Test
        func graphWithNodesWithPendingEdges() async {
            await #expect(processExitsWith: .success) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let (attribute1, attribute2) = subgraph.apply {
                    let attribute1 = Attribute(value: 1)
                    let attribute2 = Attribute(TestRule(input: attribute1))
                    return (attribute1, attribute2)
                }

                let _ = attribute2.value
                attribute1.invalidateValue()

                let description = graph.graphvizDescription(includeValues: false)
                #expect(
                    description == """
                        digraph {
                          _576[label="576: GraphDescriptionTests.GraphvizTests.Test…" style="bold" color=red];
                          _536 -> _576[ color=red];
                          _536[label="536: External value" style="bold"];
                        }

                        """
                )
            }
        }

        @Test
        func graphWithNodesIncludingValues() async {
            await #expect(processExitsWith: .success) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let attribute2 = subgraph.apply {
                    let attribute1 = Attribute(value: 1)
                    return Attribute(TestRule(input: attribute1))
                }

                let _ = attribute2.value

                let description = graph.graphvizDescription(includeValues: true)
                #expect(
                    description == """
                        digraph {
                          _576[label="576: GraphDescriptionTests.GraphvizTests.Test… → 2" style="bold"];
                          _536 -> _576[];
                          _536[label="536: External value → 1" style="bold"];
                        }

                        """
                )
            }
        }
    }

    @Suite
    struct StackTests {
        struct StackDescriptionConfirmingRule: Rule {
            var expectedDescription: String
            var confirmation: Confirmation

            var value: Int {
                let description = Graph.stackDescription(maxFrames: 8)
                if description == expectedDescription {
                    confirmation.confirm()
                }
                return 1
            }
        }

        @Test
        func stackDescription() async {
            await #expect(processExitsWith: .success) {
                await confirmation(expectedCount: 1) { confirmation in
                    let graph = Graph()
                    let subgraph = Subgraph(graph: graph)
                    let attribute = subgraph.apply {
                        Attribute(
                            StackDescriptionConfirmingRule(
                                expectedDescription: """
                                      #0: 536 StackDescriptionConfirmingRule -> Int
                                    
                                    """,
                                confirmation: confirmation
                            )
                        )
                    }
                    
                    let _ = attribute.value
                }
            }
        }

        @Test
        func stackDescriptionIsEmptyWhenNotUpdating() async {
            struct TestRule: Rule {
                var value: Int { 1 }
            }
            
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule())
            }

            let _ = attribute.value

            let description = Graph.stackDescription(maxFrames: 8)
            #expect(description == "")
        }
    }
}
#endif
