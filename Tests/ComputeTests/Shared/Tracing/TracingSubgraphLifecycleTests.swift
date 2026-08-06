import Testing

@Suite
struct TracingSubgraphLifecycleTests {
    @Suite
    struct SubgraphCreatedTests {
        @Test
        func traceSubgraphCreatedCalled() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            #expect(recorder.history.subgraphCreatedEntries.count == 0)

            let subgraph = Subgraph(graph: graph)

            let subgraphCreatedEntries = recorder.history.subgraphCreatedEntries
            #expect(subgraphCreatedEntries.count == 1)
            #expect(subgraphCreatedEntries[0].subgraph == subgraph)
        }
    }

    @Suite
    struct SubgraphInvalidateTests {
        @Test
        func traceSubgraphDestroyCalledForOnSubgraphInvalidate() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)

            #expect(recorder.history.subgraphDestroyEntries.count == 0)

            subgraph.invalidate()

            let subgraphDestroyEntries = recorder.history.subgraphDestroyEntries
            #expect(subgraphDestroyEntries.count == 1)
            #expect(subgraphDestroyEntries[0].subgraph == subgraph)
        }
    }

    @Suite
    struct SubgraphDestroyTests {
        @Test
        func traceSubgraphDestroyNotCalledOnSubgraphDeinit() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            #expect(recorder.history.subgraphDestroyEntries.count == 0)

            autoreleasepool {
                let _ = Subgraph(graph: graph)
            }

            let subgraphDestroyEntries = recorder.history.subgraphDestroyEntries
            #expect(subgraphDestroyEntries.count == 0)  // Not called when Subgraph deinit is called
        }
    }

    @Suite
    struct SubgraphAddChildTests {
        @Test
        func traceSubgraphAddChildCalled() {
            class SubgraphTrace: TestTraceRecorder {
                override func subgraphAddChild(subgraph: Subgraph, childSubgraph: Subgraph) {
                    super.subgraphAddChild(subgraph: subgraph, childSubgraph: childSubgraph)

                    #expect(subgraph.childCount == 0)  // "will" semantics
                    #expect(childSubgraph.parentCount == 0)  // "will" semantics
                }
            }

            let graph = Graph()
            let recorder = SubgraphTrace()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let childSubgraph = Subgraph(graph: graph)
            subgraph.addChild(childSubgraph)

            #expect(subgraph.childCount == 1)
            #expect(childSubgraph.parentCount == 1)

            let subgraphAddChildEntries = recorder.history.subgraphAddChildEntries
            #expect(subgraphAddChildEntries.count == 1)
            #expect(subgraphAddChildEntries[0].subgraph == subgraph)
            #expect(subgraphAddChildEntries[0].childSubgraph == childSubgraph)
        }
    }

    @Suite
    struct SubgraphRemoveChildTests {
        @Test
        func traceSubgraphRemoveChildCalled() {
            class SubgraphTrace: TestTraceRecorder {
                override func subgraphRemoveChild(subgraph: Subgraph, childSubgraph: Subgraph) {
                    super.subgraphRemoveChild(subgraph: subgraph, childSubgraph: childSubgraph)

                    #expect(subgraph.childCount == 1)  // "will" semantics
                    #expect(childSubgraph.parentCount == 0)  // "did" semantics
                }
            }

            let graph = Graph()
            let recorder = SubgraphTrace()
            recorder.install(graph: graph)

            #expect(recorder.history.subgraphRemoveChildEntries.count == 0)

            let subgraph = Subgraph(graph: graph)
            let childSubgraph = Subgraph(graph: graph)
            subgraph.addChild(childSubgraph)
            subgraph.removeChild(childSubgraph)

            let subgraphRemoveChildEntries = recorder.history.subgraphRemoveChildEntries
            #expect(subgraphRemoveChildEntries.count == 1)
            #expect(subgraphRemoveChildEntries[0].subgraph == subgraph)
            #expect(subgraphRemoveChildEntries[0].childSubgraph == childSubgraph)
        }
    }
}
