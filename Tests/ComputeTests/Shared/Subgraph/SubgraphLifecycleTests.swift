import Testing

@Suite
struct SubgraphLifecycleTests {
    @Suite
    struct SubgraphCreatedTests {
        @Test
        func addedToGraph() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.graph == graph)
        }
        
        @Test
        func graphCountersIncremented() {
            let graph = Graph()
            
            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 0)
            
            let subgraph = Subgraph(graph: graph)
            withExtendedLifetime(subgraph) {
                #expect(graph.counter(for: .subgraphs) == 1)
                #expect(graph.counter(for: .createdSubgraphs) == 1)
            }
        }
        
        @Test
        func traceSubgraphCreatedCalled() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            #expect(recorder.history.subgraphCreatedEntries.count == 0)

            let subgraph = Subgraph(graph: graph)

            let subgraphCreatedEntries = recorder.history.subgraphCreatedEntries
            #expect(subgraphCreatedEntries.count == 1)
            #expect(subgraphCreatedEntries[0].subgraph == subgraph)
        }
    }
    
    @Suite
    struct SubgraphInvalidateTests {
        @Test
        func removedFromGraph() async {
            await #expect(processExitsWith: .failure) {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                
                subgraph.invalidate()
                
                print(subgraph.graph) // will crash
            }
        }
        
        @Test
        func graphCountersDecrementedOnSubgraphInvalidate() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            #expect(graph.counter(for: .subgraphs) == 1)
            #expect(graph.counter(for: .createdSubgraphs) == 1)

            subgraph.invalidate()
            withExtendedLifetime(subgraph) {
                #expect(graph.counter(for: .subgraphs) == 0)
                #expect(graph.counter(for: .createdSubgraphs) == 1)
            }
        }
        
        @Test
        func traceSubgraphDestroyCalledForOnSubgraphInvalidate() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)

            #expect(recorder.history.subgraphDestroyEntries.count == 0)

            subgraph.invalidate()

            let subgraphDestroyEntries = recorder.history.subgraphDestroyEntries
            #expect(subgraphDestroyEntries.count == 1)
            #expect(subgraphDestroyEntries[0].subgraph == subgraph)
        }
    }
    
    @Suite
    struct SubgraphDestroyTests {
        // This really just tests that Subgraph.deinit was triggered
        @Test
        func removedFromGraph() async {
            let graph = Graph()
            weak var weakSubgraph: Subgraph? = nil
            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                weakSubgraph = subgraph
            }
            #expect(weakSubgraph == nil)
        }
        
        @Test
        func graphCountersDecrementedOnSubgraphDeinit() {
            let graph = Graph()
            
            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 0)

            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                withExtendedLifetime(subgraph) {
                    #expect(graph.counter(for: .subgraphs) == 1)
                    #expect(graph.counter(for: .createdSubgraphs) == 1)
                }
            }

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 1)
        }
        
        @Test
        func traceSubgraphDestroyNotCalledOnSubgraphDeinit() {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            #expect(recorder.history.subgraphDestroyEntries.count == 0)

            autoreleasepool {
                let _ = Subgraph(graph: graph)
            }

            let subgraphDestroyEntries = recorder.history.subgraphDestroyEntries
            #expect(subgraphDestroyEntries.count == 0) // Not called when Subgraph deinit is called
        }
    }
    
    @Suite
    struct SubgraphAddChildTests {
        @Test
        func traceSubgraphAddChildCalled() {
            class SubgraphTrace: TestTraceRecorder {
                override func subgraphAddChild(subgraph: Subgraph, childSubgraph: Subgraph) {
                    super.subgraphAddChild(subgraph: subgraph, childSubgraph: childSubgraph)

                    #expect(subgraph.childCount == 0)  // "will" semantics
                    #expect(childSubgraph.parentCount == 0)  // "will" semantics
                }
            }

            let graph = Graph()
            let recorder = SubgraphTrace()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let childSubgraph = Subgraph(graph: graph)
            subgraph.addChild(childSubgraph)

            #expect(subgraph.childCount == 1)
            #expect(childSubgraph.parentCount == 1)

            let subgraphAddChildEntries = recorder.history.subgraphAddChildEntries
            #expect(subgraphAddChildEntries.count == 1)
            #expect(subgraphAddChildEntries[0].subgraph == subgraph)
            #expect(subgraphAddChildEntries[0].childSubgraph == childSubgraph)
        }
    }
    
    @Suite
    struct SubgraphRemoveChildTests {
        @Test
        func traceSubgraphRemoveChildCalled() {
            class SubgraphTrace: TestTraceRecorder {
                override func subgraphRemoveChild(subgraph: Subgraph, childSubgraph: Subgraph) {
                    super.subgraphRemoveChild(subgraph: subgraph, childSubgraph: childSubgraph)

                    #expect(subgraph.childCount == 1)  // "will" semantics
                    #expect(childSubgraph.parentCount == 0)  // "did" semantics
                }
            }

            let graph = Graph()
            let recorder = SubgraphTrace()
            recorder.install(graph: graph)

            #expect(recorder.history.subgraphRemoveChildEntries.count == 0)

            let subgraph = Subgraph(graph: graph)
            let childSubgraph = Subgraph(graph: graph)
            subgraph.addChild(childSubgraph)
            subgraph.removeChild(childSubgraph)

            let subgraphRemoveChildEntries = recorder.history.subgraphRemoveChildEntries
            #expect(subgraphRemoveChildEntries.count == 1)
            #expect(subgraphRemoveChildEntries[0].subgraph == subgraph)
            #expect(subgraphRemoveChildEntries[0].childSubgraph == childSubgraph)
        }
    }
}
