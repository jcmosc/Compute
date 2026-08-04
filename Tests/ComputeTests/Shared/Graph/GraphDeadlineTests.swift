import Testing
import _ComputeTestSupport

@Suite
struct GraphDeadlineTests {
    @Test
    func withDeadline() {
        let graph = Graph()
        
        let trace = TestTrace()
        trace.register(graph: graph)
        
        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(0) {
            #expect(graph.deadline == 0)
        }
        #expect(graph.deadline == UInt64.max)
        
        let setDeadlineEvents = trace.events(of: .setDeadline)
        #expect(setDeadlineEvents.count == 2)
        #expect(setDeadlineEvents[0].message == "deadline = 0")
        #expect(setDeadlineEvents[1].message == "deadline = \(UInt64.max)")
    }
    
    @Test
    func withDeadlineNonzero() {
        let graph = Graph()
        
        let trace = TestTrace()
        trace.register(graph: graph)
        
        #expect(graph.deadline == UInt64.max)
        graph.withDeadline(100) {
            #expect(graph.deadline == 100)
        }
        #expect(graph.deadline == UInt64.max)
        
        let setDeadlineEvents = trace.events(of: .setDeadline)
        #expect(setDeadlineEvents.count == 2)
        #expect(setDeadlineEvents[0].message == "deadline = 100")
        #expect(setDeadlineEvents[1].message == "deadline = \(UInt64.max)")
    }
}
