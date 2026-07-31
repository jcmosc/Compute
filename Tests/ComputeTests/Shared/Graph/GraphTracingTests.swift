import Testing

@Suite
struct GraphTracingTests {
    @Test
    func addTrace() {
        class Context {
            var traceCalls: [(name: String, graph: Graph)] = []
        }

        var trace = TraceType()
        trace.begin_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "beginTrace", graph: graph))
            }
        }
        trace.end_trace = { contextPointer, graph in
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

        var trace = TraceType()
        trace.begin_trace = { contextPointer, graph in
            if let context = contextPointer?.assumingMemoryBound(to: Context.self).pointee {
                context.traceCalls.append((name: "beginTrace", graph: graph))
            }
        }
        trace.end_trace = { contextPointer, graph in
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
