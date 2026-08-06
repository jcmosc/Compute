import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingGraphInvalidationTests {
    struct TestRule: Rule {
        @Attribute var input: Int
        var value: Int { return input + 1 }
    }
    
    @Suite
    struct BeginGraphInvalidationTests {
        // In order for `beginGraphInvalidation()` to be called:
        //   - The graph mush have an invalidation callback
        //   - The node's output edges must be non-empty
        //   - The node's inputs must traverse contexts
        
        @Test
        func traceBeginGraphInvalidationCalled() throws {
            let globalGraph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: globalGraph)

            let inputGraph = Graph(shared: globalGraph)
            let inputSubgraph = Subgraph(graph: inputGraph)
            let input = inputSubgraph.apply {
                Attribute(value: 1)
            }

            let outputGraph = Graph(shared: globalGraph)
            let outputSubgraph = Subgraph(graph: outputGraph)
            let output = outputSubgraph.apply {
                let attribute = Attribute(TestRule(input: input))
                return Attribute(TestRule(input: attribute))
            }
            
            outputGraph.onInvalidation { _ in }

            // Establish node edges
            let _ = output.value

            try #require(recorder.history.beginGraphInvalidationEntries.count == 0)

            input.invalidateValue()

            let beginGraphInvalidationEntries = recorder.history.beginGraphInvalidationEntries
            try #require(beginGraphInvalidationEntries.count == 1)
            #expect(beginGraphInvalidationEntries[0].graph == outputGraph)
            #expect(beginGraphInvalidationEntries[0].attribute == input.identifier)
        }
        
        @Test
        func traceBeginGraphInvalidationNotCalledWhenNoInvalidationCallback() throws {
            let globalGraph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: globalGraph)

            let inputGraph = Graph(shared: globalGraph)
            let inputSubgraph = Subgraph(graph: inputGraph)
            let input = inputSubgraph.apply {
                Attribute(value: 1)
            }

            let outputGraph = Graph(shared: globalGraph)
            let outputSubgraph = Subgraph(graph: outputGraph)
            let output = outputSubgraph.apply {
                let attribute = Attribute(TestRule(input: input))
                return Attribute(TestRule(input: attribute))
            }
            
            // No invalidation callback
            
            // establish node edges
            let _ = output.value

            try #require(recorder.history.beginGraphInvalidationEntries.count == 0)

            input.invalidateValue()

            #expect(recorder.history.beginGraphInvalidationEntries.count == 0)
        }

        @Test
        func traceBeginGraphInvalidationNotCalledWhenNoOutputEdge() throws {
            let globalGraph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: globalGraph)

            let inputGraph = Graph(shared: globalGraph)
            let inputSubgraph = Subgraph(graph: inputGraph)
            let input = inputSubgraph.apply {
                Attribute(value: 1)
            }

            let outputGraph = Graph(shared: globalGraph)
            let outputSubgraph = Subgraph(graph: outputGraph)
            let attribute = outputSubgraph.apply {
                Attribute(TestRule(input: input))
                // No output edge
            }
            
            outputGraph.onInvalidation { _ in }
            
            // Establish node edges
            let _ = attribute.value

            try #require(recorder.history.beginGraphInvalidationEntries.count == 0)

            input.invalidateValue()

            #expect(recorder.history.beginGraphInvalidationEntries.count == 0)
        }

        @Test
        func traceBeginGraphInvalidationNotCalledWhenInputsDoNotTraverseContexts() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            graph.onInvalidation { _ in }

            let subgraph = Subgraph(graph: graph)
            let (input, output) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: input))
                let output = Attribute(TestRule(input: attribute))
                return (input, output)
            }
            
            // Establish node edges
            let _ = output.value

            try #require(recorder.history.beginGraphInvalidationEntries.count == 0)

            input.invalidateValue()

            #expect(recorder.history.beginGraphInvalidationEntries.count == 0)
        }
    }

    @Suite
    struct EndGraphInvalidationTests {
        @Test
        func traceEndGraphInvalidationCalled() throws {
            let globalGraph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: globalGraph)

            let inputGraph = Graph(shared: globalGraph)
            let inputSubgraph = Subgraph(graph: inputGraph)
            let input = inputSubgraph.apply {
                Attribute(value: 1)
            }

            let outputGraph = Graph(shared: globalGraph)
            let outputSubgraph = Subgraph(graph: outputGraph)
            let output = outputSubgraph.apply {
                let attribute = Attribute(TestRule(input: input))
                return Attribute(TestRule(input: attribute))
            }
            
            outputGraph.onInvalidation { _ in }

            // Establish node edges
            let _ = output.value

            try #require(recorder.history.endGraphInvalidationEntries.count == 0)

            input.invalidateValue()

            let endGraphInvalidationEntries = recorder.history.endGraphInvalidationEntries
            try #require(endGraphInvalidationEntries.count == 1)
            #expect(endGraphInvalidationEntries[0].graph == outputGraph)
            #expect(endGraphInvalidationEntries[0].attribute == input.identifier)
        }
    }
}
