import Testing

@Suite
struct TracingCustomEventTests {
    @Suite
    struct CustomEventTests {
        @Test
        func traceCustomEventCalled() throws {
            class CustomEventTrace: TestTraceRecorder {
                var capturedValue: Int?
                var capturedType: Any.Type?

                override func customEvent(graph: Graph, eventName: String, value: UnsafeRawPointer, type: Any.Type) {
                    super.customEvent(graph: graph, eventName: eventName, value: value, type: type)
                    capturedValue = value.assumingMemoryBound(to: Int.self).pointee
                    capturedType = type
                }
            }

            let graph = Graph()
            let recorder = CustomEventTrace()
            recorder.install(graph: graph)

            try #require(recorder.history.customEventEntries.count == 0)

            "custom_event".withCString { eventName in
                graph.addTraceEvent(eventName, value: 42)
            }

            let customEventEntries = recorder.history.customEventEntries
            try #require(customEventEntries.count == 1)
            #expect(customEventEntries[0].graph == graph)
            #expect(customEventEntries[0].eventName == "custom_event")
            #expect(ObjectIdentifier(customEventEntries[0].type) == ObjectIdentifier(Int.self))
            // value pointer is no longer value here
            #expect(recorder.capturedValue == 42)
            #expect(recorder.capturedType == Int.self)
        }

        @Test
        func traceCustomEventCalledWithContext() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            try #require(recorder.history.customEventEntries.count == 0)

            let value: Int = 42
            try withUnsafePointer(to: value) { context in
                "custom_event".withCString { eventName in
                    graph.addTraceEvent(eventName, context: context)
                }

                let customEventEntries = recorder.history.customEventEntries
                try #require(customEventEntries.count == 1)
                #expect(customEventEntries[0].graph == graph)
                #expect(customEventEntries[0].eventName == "custom_event")
                #expect(customEventEntries[0].value == context)
                #expect(ObjectIdentifier(customEventEntries[0].type) == ObjectIdentifier(Int.self))
            }
        }
    }
}
