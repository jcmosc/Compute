import Testing

@Suite
struct TracingTraceTests {
    @Suite
    struct BeginTraceTests {
        @Test
        func traceBeginTraceCalled() throws {
            let graph = Graph()            

            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let beginTraceEntries = recorder.history.beginTraceEntries
            try #require(beginTraceEntries.count == 1)
            #expect(beginTraceEntries[0].graph == graph)
        }
    }

    @Suite
    struct EndTraceTests {
        @Test
        func traceEndTraceCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.endTraceEntries.count == 0)

            recorder.uninstall()

            let endTraceEntries = recorder.history.endTraceEntries
            try #require(endTraceEntries.count == 1)
            #expect(endTraceEntries[0].graph == graph)
        }
    }
}
