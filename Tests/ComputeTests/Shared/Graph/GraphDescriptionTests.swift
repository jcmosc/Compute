import Foundation
import Testing
import _ComputeTestSupport

#if canImport(Darwin)
@Suite(.serialized(for: \GraphHost.Type.sharedGraph))
struct GraphDescriptionTests {
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
        func initialDescription() async throws {
            try await #require(processExitsWith: .success) {
                let description =
                    try #require(
                        Graph.description(
                            nil,
                            // TODO: Conform DescriptionOption to CustomStringConvertible so we don't have to access rawValue here
                            options: [DescriptionOption.format.rawValue: "graph/dict"] as NSDictionary
                        )
                            as? NSDictionary
                    )

                let json = try JSONSerialization.data(
                    withJSONObject: description,
                    options: [.prettyPrinted, .sortedKeys]
                )
                let jsonString = try #require(String(data: json, encoding: .utf8))
                assertStringsEqualWithDiff(
                    jsonString,
                    """
                    {
                      "counters" : {
                        "bytes" : 0,
                        "max_bytes" : 0
                      },
                      "graphs" : [

                      ],
                      "version" : 2
                    }
                    """
                )
            }
        }

        @Test
        func graphDescription() async throws {
            try await #require(processExitsWith: .success) {
                let graph = Graph()

                let subgraph = Subgraph(graph: graph)
                let child = Subgraph(graph: graph)
                subgraph.addChild(child, tag: 1)

                let description = graph.dictionaryDescription()
                let expectedDescription = Graph.DictionaryDescription(
                    version: 2,
                    counters: .init(bytes: 0, maxBytes: 0),
                    graphs: [
                        Graph.DictionaryDescription.Graph(
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
                            subgraphs: [
                                .init(id: 1, contextID: 2, children: [1]),
                                .init(id: 2, contextID: 2, parents: [0]),
                            ],
                            updateCount: 0,
                            changeCount: 0,
                            transactionCount: 0
                        )
                    ]
                )

                // FIXME: post-process the subgraphs array in the description output.
                withKnownIssue(
                    "Subgraphs is sorted by pointer address, which we can't predict deterministically.",
                    isIntermittent: true
                ) {
                    assertValuesEqualWithDiff(description, expectedDescription)
                }
            }
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
                        == "GraphDescriptionTests.BodyDescriptionTests.TestBody"
                )
            }
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
}
#endif
