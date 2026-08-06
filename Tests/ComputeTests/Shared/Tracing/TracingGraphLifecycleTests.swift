import Testing

@Suite
struct TracingGraphLifecycleTests {
    @Suite
    struct GraphCreatedTests {
        @Test
        func traceGraphCreatedCalledForSharedGraph() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.graphCreatedEntries.count == 0)

            let sharedGraph = Graph(shared: graph)

            let graphCreatedEntries = recorder.history.graphCreatedEntries
            try #require(graphCreatedEntries.count == 1)
            #expect(graphCreatedEntries[0].graph == sharedGraph)
        }
    }

    @Suite
    struct GraphDestroyTests {
        @Test
        func traceGraphDestroyCalledOnInvalidate() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.graphDestroyEntries.count == 0)

            graph.invalidate()

            let graphDestroyEntries = recorder.history.graphDestroyEntries
            try #require(graphDestroyEntries.count == 1)
            #expect(graphDestroyEntries[0].graph == graph)
        }
    }

    @Suite
    struct GraphNeedsUpdateTests {
        @Test
        func traceGraphNeedsUpdateCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.graphNeedsUpdateEntries.count == 0)

            graph.setNeedsUpdate()

            let graphNeedsUpdateEntries = recorder.history.graphNeedsUpdateEntries
            try #require(graphNeedsUpdateEntries.count == 1)
            #expect(graphNeedsUpdateEntries[0].graph == graph)
        }
    }
}
