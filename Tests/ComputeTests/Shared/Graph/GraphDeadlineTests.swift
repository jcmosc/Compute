import Testing
import _ComputeTestSupport

@Suite
struct GraphDeadlineTests {
    @Test
    func withDeadline() {
        let graph = Graph()

        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(0) {
            #expect(graph.deadline == 0)
        }
        #expect(graph.deadline == UInt64.max)
    }

    @Test
    func withDeadlineNonzero() {
        let graph = Graph()

        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(100) {
            #expect(graph.deadline == 100)
        }
        #expect(graph.deadline == UInt64.max)
    }
}
