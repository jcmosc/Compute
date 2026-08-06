import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingUpdateTests {
    struct TestRule: Rule {
        @Attribute var input: Int
        var value: Int { 1 }
    }

    @Suite
    struct BeginSubgraphUpdateTests {
        @Test
        func traceBeginSubgraphUpdateCalledOnSubgraphUpdateWithDirtyFlags() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input), initialValue: 0)
            }

            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: Subgraph.Flags(rawValue: 1))
            attribute.invalidateValue()

            try #require(recorder.history.beginSubgraphUpdateEntries.count == 0)

            subgraph.update(flags: Subgraph.Flags(rawValue: 1))

            let beginSubgraphUpdateEntries = recorder.history.beginSubgraphUpdateEntries
            try #require(beginSubgraphUpdateEntries.count == 1)
            #expect(beginSubgraphUpdateEntries[0].subgraph == subgraph)
            #expect(beginSubgraphUpdateEntries[0].flags == Subgraph.Flags(rawValue: 1))
        }

        @Test
        func traceBeginSubgraphUpdateNotCalledOnSubgraphUpdateWithNonDirtyFlags() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input), initialValue: 0)
            }

            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: Subgraph.Flags(rawValue: 1))
            attribute.invalidateValue()

            try #require(recorder.history.beginSubgraphUpdateEntries.count == 0)

            subgraph.update(flags: Subgraph.Flags(rawValue: 2))  // won't match attribute

            try #require(recorder.history.beginSubgraphUpdateEntries.count == 0)
        }

        @Test
        func traceBeginSubgraphUpdateNotCalledOnSubgraphUpdateWithEmptyFlags() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input), initialValue: 0)
            }

            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: Subgraph.Flags(rawValue: 1))
            attribute.invalidateValue()

            try #require(recorder.history.beginSubgraphUpdateEntries.count == 0)

            subgraph.update(flags: [])

            try #require(recorder.history.beginSubgraphUpdateEntries.count == 0)
        }
    }

    @Suite
    struct EndSubgraphUpdateTests {
        @Test
        func traceEndSubgraphUpdateCalledOnSubgraphUpdateWithDirtyFlags() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input), initialValue: 0)
            }

            attribute.setFlags(Subgraph.Flags(rawValue: 1), mask: Subgraph.Flags(rawValue: 1))
            attribute.invalidateValue()

            try #require(recorder.history.endSubgraphUpdateEntries.count == 0)

            subgraph.update(flags: Subgraph.Flags(rawValue: 1))

            let endSubgraphUpdateEntries = recorder.history.endSubgraphUpdateEntries
            try #require(endSubgraphUpdateEntries.count == 1)
            #expect(endSubgraphUpdateEntries[0].subgraph == subgraph)
        }
    }

    @Suite
    struct BeginNodeUpdateTests {
        @Test
        func traceBeginNodeUpdateCalledOnReadValue() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.beginNodeUpdateEntries.count == 0)

            let _ = attribute.value

            let beginNodeUpdateEntries = recorder.history.beginNodeUpdateEntries
            try #require(beginNodeUpdateEntries.count == 1)
            #expect(beginNodeUpdateEntries[0].attribute == attribute.identifier)
        }
    }

    @Suite
    struct EndNodeUpdateTests {
        @Test
        func traceEndNodeUpdateCalledOnReadValue() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.endNodeUpdateEntries.count == 0)

            let _ = attribute.value

            let endNodeUpdateEntries = recorder.history.endNodeUpdateEntries
            try #require(endNodeUpdateEntries.count == 1)
            #expect(endNodeUpdateEntries[0].attribute == attribute.identifier)
            #expect(endNodeUpdateEntries[0].changed == true)
        }

        @Test
        func traceEndNodeUpdateCalledOnReadValueWithAbortedUpdate() throws {
            struct CancellingRule: Rule {
                var value: Int {
                    Graph.cancelUpdate()
                    return 0
                }
            }

            struct ReadingRule: Rule {
                @Attribute var input: Int
                var value: Int { input + 1 }
            }

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let (attribute, dependency) = subgraph.apply {
                let source = Attribute(value: 42)
                let indirect = IndirectAttribute(source: source)
                let dependency = Attribute(CancellingRule())
                indirect.dependency = dependency.identifier
                let attribute = Attribute(ReadingRule(input: indirect.attribute))
                return (attribute, dependency)
            }

            // Establish node edges
            let _ = attribute.value
            var endNodeUpdateEntries = recorder.history.endNodeUpdateEntries.filter {
                $0.attribute == attribute.identifier
            }
            try #require(endNodeUpdateEntries.count == 1)
            #expect(endNodeUpdateEntries[0].changed == true)  // The value was read with default update options

            dependency.invalidateValue()
            attribute.prefetchValue()  // Uses `.abortIfCancelled | .cancelIfPassedDeadline`

            endNodeUpdateEntries = recorder.history.endNodeUpdateEntries.filter {
                $0.attribute == attribute.identifier
            }
            try #require(endNodeUpdateEntries.count == 2)
            #expect(endNodeUpdateEntries[1].attribute == attribute.identifier)
            // The update was cancelled and `.abortIfCancelled` was specified
            #expect(endNodeUpdateEntries[1].changed == false)
        }
    }

    @Suite
    struct BeginValueUpdateTests {
        @Test
        func traceBeginValueUpdateCalledOnReadValue() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.beginValueUpdateEntries.count == 0)

            let _ = attribute.value

            let beginValueUpdateEntries = recorder.history.beginValueUpdateEntries
            try #require(beginValueUpdateEntries.count == 1)
            #expect(beginValueUpdateEntries[0].attribute == attribute.identifier)
        }
    }

    @Suite
    struct EndValueUpdateTests {
        @Test
        func traceEndValueUpdateCalledOnValueRead() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.endValueUpdateEntries.count == 0)

            let _ = attribute.value

            let endValueUpdateEntries = recorder.history.endValueUpdateEntries
            try #require(endValueUpdateEntries.count == 1)
            #expect(endValueUpdateEntries[0].attribute == attribute.identifier)
            #expect(endValueUpdateEntries[0].changed == true)
        }

        @Test
        func traceEndValueUpdateCalledOnReadValueWithNoChange() throws {
            struct ConstantRule: Rule {
                var value: Int { 42 }
            }

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(ConstantRule())
            }

            let _ = attribute.value
            try #require(recorder.history.endValueUpdateEntries.count == 1)
            try #require(recorder.history.endValueUpdateEntries[0].changed == true)

            // Trigger update with no change
            attribute.invalidateValue()
            let _ = attribute.value

            let endValueUpdateEntries = recorder.history.endValueUpdateEntries
            try #require(endValueUpdateEntries.count == 2)
            #expect(endValueUpdateEntries[1].attribute == attribute.identifier)
            #expect(endValueUpdateEntries[1].changed == false)
        }
    }

    @Suite
    struct BeginGraphUpdateTests {
        @Test
        func traceBeginGraphUpdateCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            graph.onUpdate {}
            graph.setNeedsUpdate()

            try #require(recorder.history.beginGraphUpdateEntries.count == 0)

            let _ = attribute.value

            let beginGraphUpdateEntries = recorder.history.beginGraphUpdateEntries
            try #require(beginGraphUpdateEntries.count == 1)
            #expect(beginGraphUpdateEntries[0].graph == graph)
        }

        @Test
        func traceBeginGraphUpdateNotCalledWhenGraphHasNoUpdateHandler() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.beginGraphUpdateEntries.count == 0)

            let _ = attribute.value

            let beginGraphUpdateEntries = recorder.history.beginGraphUpdateEntries
            try #require(beginGraphUpdateEntries.count == 0)
        }
    }

    @Suite
    struct EndGraphUpdateTests {
        @Test
        func traceEndGraphUpdateCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            graph.onUpdate {}
            graph.setNeedsUpdate()

            try #require(recorder.history.endGraphUpdateEntries.count == 0)

            let _ = attribute.value

            let endGraphUpdateEntries = recorder.history.endGraphUpdateEntries
            try #require(endGraphUpdateEntries.count == 1)
            #expect(endGraphUpdateEntries[0].graph == graph)
        }

        @Test
        func traceEndGraphUpdateNotCalledWhenGraphHasNoUpdateHandler() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            try #require(recorder.history.endGraphUpdateEntries.count == 0)

            let _ = attribute.value

            let endGraphUpdateEntries = recorder.history.endGraphUpdateEntries
            try #require(endGraphUpdateEntries.count == 0)
        }
    }
}
