import Testing
import _ComputeTestSupport

@Suite
struct GraphDeadlineTests {
    @Test
    func withDeadline() {
        let graph = Graph()
        let recorder = TestTraceRecorder()
        recorder.install(graph: graph)

        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(0) {
            #expect(graph.deadline == 0)
        }
        #expect(graph.deadline == UInt64.max)

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

        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(100) {
            #expect(graph.deadline == 100)
        }
        #expect(graph.deadline == UInt64.max)

        let entries: [TestTraceRecorder.History.SetDeadlineEntry] = recorder.history.entries.compactMap { event in
            guard case .setDeadline(let entry) = event else { return nil }
            return entry
        }
        #expect(entries.count == 2)
        #expect(entries[0].deadline == 100)
        #expect(entries[1].deadline == UInt.max)
    }
}
