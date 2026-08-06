import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingIndirectNodeLifecycleTests {
    @Suite
    struct IndirectNodeAddedTests {
        @Test
        func traceIndirectNodeAddedCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let source = subgraph.apply {
                Attribute(value: 1)
            }

            try #require(recorder.history.indirectNodeAddedEntries.count == 0)

            let indirect = subgraph.apply {
                IndirectAttribute(source: source)
            }

            let indirectNodeAddedEntries = recorder.history.indirectNodeAddedEntries
            try #require(indirectNodeAddedEntries.count == 1)
            #expect(indirectNodeAddedEntries[0].attribute == indirect.identifier)
        }
    }

    @Suite
    struct IndirectNodeSetSourceTests {
        @Test
        func traceIndirectNodeSetSourceCalledOnSetSource() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let indirect = subgraph.apply {
                let source1 = Attribute(value: 1)
                return IndirectAttribute(source: source1)
            }

            try #require(recorder.history.indirectNodeSetSourceEntries.count == 0)
            
            let source2 = subgraph.apply {
                Attribute(value: 2)
            }

            indirect.source = source2

            let indirectNodeSetSourceEntries = recorder.history.indirectNodeSetSourceEntries
            try #require(indirectNodeSetSourceEntries.count == 1)
            #expect(indirectNodeSetSourceEntries[0].attribute == indirect.identifier)
            #expect(indirectNodeSetSourceEntries[0].source == source2.identifier)
        }

        @Test
        func traceIndirectNodeSetSourceCalledOnResetSource() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (indirect, source1) = subgraph.apply {
                let source1 = Attribute(value: 1)
                let indirect = IndirectAttribute(source: source1)
                return (indirect, source1)
            }

            let source2 = subgraph.apply {
                Attribute(value: 2)
            }
            
            indirect.source = source2
            try #require(recorder.history.indirectNodeSetSourceEntries.count == 1)

            indirect.resetSource()

            let indirectNodeSetSourceEntries = recorder.history.indirectNodeSetSourceEntries
            try #require(indirectNodeSetSourceEntries.count == 2)
            #expect(indirectNodeSetSourceEntries[1].attribute == indirect.identifier)
            #expect(indirectNodeSetSourceEntries[1].source == source1.identifier)
        }
    }

    @Suite
    struct IndirectNodeSetDependencyTests {
        @Test
        func traceIndirectNodeSetDependencyCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let indirect = subgraph.apply {
                let source = Attribute(value: 1)
                return IndirectAttribute(source: source)
            }

            let dependency = subgraph.apply {
                Attribute(value: 2)
            }

            try #require(recorder.history.indirectNodeSetDependencyEntries.count == 0)

            indirect.dependency = dependency.identifier

            let indirectNodeSetDependencyEntries = recorder.history.indirectNodeSetDependencyEntries
            try #require(indirectNodeSetDependencyEntries.count == 1)
            #expect(indirectNodeSetDependencyEntries[0].attribute == indirect.identifier)
            #expect(indirectNodeSetDependencyEntries[0].dependency == dependency.identifier)
        }
    }
}
