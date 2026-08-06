import Foundation
import Testing

@Suite
struct TracingNamedEventTests {
    @Suite
    struct NamedEventTests {
        @Test
        func traceNamedEventCalled() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event", subsystem: "test_subsystem")
            
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.namedEventEntries.count == 0)

            graph.addNamedTraceEvent(eventID)

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [])
            #expect(namedEventEntries[0].data == nil)
            #expect(namedEventEntries[0].flags == [])
        }

        @Test
        func traceNamedEventCalledWithData() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_with_data", subsystem: "test_subsystem")
            
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let data = Data([0xDE, 0xAD, 0xBE, 0xEF])
            graph.addNamedTraceEvent(eventID, data: data)

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [])
            #expect(namedEventEntries[0].data == data)
            #expect(namedEventEntries[0].flags == [])
        }
        
        @Test
        func traceNamedEventCalledWithEventArgs() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_with_args", subsystem: "test_subsystem")

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.addNamedTraceEvent(eventID, eventArgs: [111, 222, 333])

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [111, 222, 333])
            #expect(namedEventEntries[0].data == nil)
            #expect(namedEventEntries[0].flags == [])
        }
        
        @Test
        func traceNamedEventCalledWithFlags() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_with_flags", subsystem: "test_subsystem")

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.addNamedTraceEvent(eventID, flags: .init(rawValue: 7))

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [])
            #expect(namedEventEntries[0].data == nil)
            #expect(namedEventEntries[0].flags == .init(rawValue: 7))
        }
        
        @Test
        func traceNamedEventCalledWithRecordBacktraceFlag() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_with_flags", subsystem: "test_subsystem")

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.addNamedTraceEvent(eventID, flags: [.recordBacktrace, .init(rawValue: 7)])

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [])
            #expect(namedEventEntries[0].data == nil)
            #expect(namedEventEntries[0].flags == [.recordBacktrace, .init(rawValue: 7)])
        }
        
        // `named_event_enabled` only affects built-in trace types such as the trace recorder.
        @Test
        func traceNamedEventCalledWhenNotEnabled() throws {
            class DisabledEvents: TestTraceRecorder {
                override func namedEventEnabled(eventID: Graph.NamedTraceEventID) -> Bool {
                    let _ = super.namedEventEnabled(eventID: eventID)
                    return false
                }
            }
            
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_disabled", subsystem: "test_subsystem")
            
            let graph = Graph()
            let recorder = DisabledEvents()
            recorder.install(graph: graph)

            try #require(recorder.history.namedEventEntries.count == 0)

            graph.addNamedTraceEvent(eventID)

            let namedEventEntries = recorder.history.namedEventEntries
            try #require(namedEventEntries.count == 1)
            #expect(namedEventEntries[0].graph == graph)
            #expect(namedEventEntries[0].eventID == eventID)
            #expect(namedEventEntries[0].eventArgs == [])
            #expect(namedEventEntries[0].data == nil)
            #expect(namedEventEntries[0].flags == [])
        }
    }

    @Suite
    struct NamedEventEnabledTests {
        @Test
        func traceNamedEventEnabledCalled() throws {
            let eventID = Graph.registerNamedTraceEvent(name: "test_event_enabled", subsystem: "test_subsystem")
            
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.namedEventEnabledEntries.count == 0)

            let _ = graph.traceEventEnabled(for: eventID.rawValue)

            let namedEventEnabledEntries = recorder.history.namedEventEnabledEntries
            try #require(namedEventEnabledEntries.count == 1)
            #expect(namedEventEnabledEntries[0].eventID == eventID)
        }
    }
}
