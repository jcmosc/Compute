import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingDeadlineTests {
    @Suite
    struct SetDeadlineTests {
        @Test
        func traceSetDeadlineCalled() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.withDeadline(0) {}

            let entries: [TestTraceRecorder.History.SetDeadlineEntry] = recorder.history.entries.compactMap { event in
                guard case .setDeadline(let entry) = event else { return nil }
                return entry
            }
            #expect(entries.count == 2)
            #expect(entries[0].deadline == 0)
            #expect(entries[1].deadline == UInt.max)
        }

        @Test
        func withDeadlineNonzero() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.withDeadline(100) {}

            let entries: [TestTraceRecorder.History.SetDeadlineEntry] = recorder.history.entries.compactMap { event in
                guard case .setDeadline(let entry) = event else { return nil }
                return entry
            }
            #expect(entries.count == 2)
            #expect(entries[0].deadline == 100)
            #expect(entries[1].deadline == UInt.max)
        }
    }

    @Suite
    struct PassedDeadlineTests {
        @Test
        func tracePassedDeadlineCalled() throws {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            Subgraph.current = subgraph
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let attribute = Attribute(value: 0)

            try #require(recorder.history.passedDeadlineEntries.count == 0)

            // Deadline of 1 will be in the past
            graph.withDeadline(1) {
                let _ = attribute.prefetchValue()
            }

            let passedDeadlineEntries = recorder.history.passedDeadlineEntries
            try #require(passedDeadlineEntries.count == 1)
        }
    }
}
