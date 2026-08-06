import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingNodeLifecycleTests {
    @Suite
    struct NodeAddedTests {
        @Test
        func traceNodeAddedCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)

            try #require(recorder.history.nodeAddedEntries.count == 0)

            let attribute = subgraph.apply {
                Attribute(value: 1)
            }

            let nodeAddedEntries = recorder.history.nodeAddedEntries
            try #require(nodeAddedEntries.count == 1)
            #expect(nodeAddedEntries[0].attribute == attribute.identifier)
        }
    }

    @Suite
    struct NodeAddEdgeTests {
        struct TestRule: Rule {
            @OptionalAttribute var input: Int?
            var value: Int { return (input ?? 0) + 1 }
        }

        @Test
        func traceNodeAddEdgeCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let attribute = Attribute(value: 0)
                let input = Attribute(value: 1)
                return (attribute, input)
            }

            try #require(recorder.history.nodeAddEdgeEntries.count == 0)

            attribute.addInput(input, options: [], token: 0)

            let nodeAddEdgeEntries = recorder.history.nodeAddEdgeEntries
            try #require(nodeAddEdgeEntries.count == 1)
            #expect(nodeAddEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeAddEdgeEntries[0].input == input.identifier)
            #expect(nodeAddEdgeEntries[0].options == [])
        }

        @Test
        func traceNodeAddEdgeCalledWithOptions() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let attribute = Attribute(value: 0)
                let input = Attribute(value: 1)
                return (attribute, input)
            }
            
            try #require(recorder.history.nodeAddEdgeEntries.count == 0)

            let options: InputOptions = [.unprefetched, .alwaysEnabled]
            attribute.addInput(input, options: options, token: 0)

            let nodeAddEdgeEntries = recorder.history.nodeAddEdgeEntries
            try #require(nodeAddEdgeEntries.count == 1)
            #expect(nodeAddEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeAddEdgeEntries[0].input == input.identifier)
            #expect(nodeAddEdgeEntries[0].options == options)
        }

        @Test
        func traceNodeAddEdgeCalledOnUpdate() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: OptionalAttribute(input)))
                return (attribute, input)
            }
            
            try #require(recorder.history.nodeAddEdgeEntries.count == 0)

            let _ = attribute.value

            let nodeAddEdgeEntries = recorder.history.nodeAddEdgeEntries
            try #require(nodeAddEdgeEntries.count == 1)
            #expect(nodeAddEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeAddEdgeEntries[0].input == input.identifier)
            #expect(nodeAddEdgeEntries[0].options == [])
        }
    }
    
    @Suite
    struct NodeRemoveEdgeTests {
        struct TestRule: Rule {
            @OptionalAttribute var input: Int?
            var value: Int { return (input ?? 0) + 1 }
        }
        
        @Test
        func traceNodeRemoveEdgeCalledOnUpdateWhenAddedExplicitly() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: OptionalAttribute(input)))
                return (attribute, input)
            }
            
            attribute.addInput(input, options: [], token: 0)
            try #require(recorder.history.nodeAddEdgeEntries.count == 1)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            attribute.mutateBody(as: TestRule.self, invalidating: true) { body in
                body.$input = nil
            }
            let _ = attribute.value

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 1)
            #expect(nodeRemoveEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeRemoveEdgeEntries[0].input == input.identifier)
        }
        
        @Test
        func traceNodeRemoveEdgeNOtCalledOnUpdateWhenAddedExplicitlyWithAlwaysEnabled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: OptionalAttribute(input)))
                return (attribute, input)
            }
            
            attribute.addInput(input, options: [.alwaysEnabled], token: 0)
            try #require(recorder.history.nodeAddEdgeEntries.count == 1)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            attribute.mutateBody(as: TestRule.self, invalidating: true) { body in
                body.$input = nil
            }
            let _ = attribute.value

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 0)
        }

        @Test
        func traceNodeRemoveEdgeCalledOnUpdateWithAddedImplicitly() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: OptionalAttribute(input)))
                return (attribute, input)
            }
            
            let _ = attribute.value
            try #require(recorder.history.nodeAddEdgeEntries.count == 1)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            attribute.mutateBody(as: TestRule.self, invalidating: true) { body in
                body.$input = nil
            }
            let _ = attribute.value

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 1)
            #expect(nodeRemoveEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeRemoveEdgeEntries[0].input == input.identifier)
        }
        
        @Test
        func traceNodeRemoveEdgeNotCalledOnSameSubgraphInvalidate() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let (attribute, _) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: OptionalAttribute(input)))
                return (attribute, input)
            }
            
            let _ = attribute.value
            try #require(recorder.history.nodeAddEdgeEntries.count == 1)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            subgraph.invalidate()

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 0)
        }
        
        @Test
        func traceNodeRemoveEdgeCalledOnDifferentSubgraphInvalidate() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let inputSubgraph = Subgraph(graph: graph)
            let input = inputSubgraph.apply {
                Attribute(value: 1)
            }

            let outputSubgraph = Subgraph(graph: graph)
            let attribute = outputSubgraph.apply {
                Attribute(TestRule(input: OptionalAttribute(input)))
            }
            
            let _ = attribute.value
            try #require(recorder.history.nodeAddEdgeEntries.count == 1)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            inputSubgraph.invalidate()

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 1)
            #expect(nodeRemoveEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeRemoveEdgeEntries[0].input == input.identifier)
        }

        @Test
        func traceNodeRemoveEdgeReportsCorrectInputAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let inputSubgraph = Subgraph(graph: graph)
            let (input1, input2) = inputSubgraph.apply {
                let input1 = Attribute(value: 1)
                let input2 = Attribute(value: 2)
                return (input1, input2)
            }

            let outputSubgraph = Subgraph(graph: graph)
            let attribute = outputSubgraph.apply {
                Attribute(value: 0)
            }
            
            attribute.addInput(input1, options: [], token: 0)
            attribute.addInput(input2, options: [], token: 1)
            try #require(recorder.history.nodeAddEdgeEntries.count == 2)
            try #require(recorder.history.nodeRemoveEdgeEntries.count == 0)

            inputSubgraph.invalidate()

            let nodeRemoveEdgeEntries = recorder.history.nodeRemoveEdgeEntries
            try #require(nodeRemoveEdgeEntries.count == 2)
            #expect(nodeRemoveEdgeEntries[0].attribute == attribute.identifier)
            #expect(nodeRemoveEdgeEntries[0].input == input2.identifier)
            #expect(nodeRemoveEdgeEntries[1].attribute == attribute.identifier)
            #expect(nodeRemoveEdgeEntries[1].input == input1.identifier)
        }
    }
    
    @Suite
    struct NodeSetEdgePendingTests {
        struct TestRule: Rule {
            var value: Int { return 0 }
        }

        @Test
        func traceNodeSetEdgePendingCalledOnInputAddedToDirtyNode() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let (attribute, input) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule())
                return (attribute, input)
            }

            try #require(recorder.history.nodeSetEdgePendingEntries.count == 0)

            attribute.invalidateValue()
            attribute.addInput(input, options: [], token: 0)

            let nodeSetEdgePendingEntries = recorder.history.nodeSetEdgePendingEntries
            try #require(nodeSetEdgePendingEntries.count == 1)
            #expect(nodeSetEdgePendingEntries[0].attribute == attribute.identifier)
            #expect(nodeSetEdgePendingEntries[0].input == input.identifier)
            #expect(nodeSetEdgePendingEntries[0].pending == true)
        }

        @Test
        func traceNodeSetEdgePendingCalledOnInputChanged() throws {
            struct TestRule: Rule {
                @Attribute var input: Int
                var value: Int { input + 1 }
            }

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let (input, attribute) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: input))
                return (input, attribute)
            }

            let _ = attribute.value
            try #require(recorder.history.nodeSetEdgePendingEntries.count == 2)

            input.value = 2

            let nodeSetEdgePendingEntries = Array(recorder.history.nodeSetEdgePendingEntries)
            try #require(nodeSetEdgePendingEntries.count == 3)
            #expect(nodeSetEdgePendingEntries[2].attribute == attribute.identifier)
            #expect(nodeSetEdgePendingEntries[2].input == input.identifier)
            #expect(nodeSetEdgePendingEntries[2].pending == true)
        }

        @Test
        func traceNodeSetEdgePendingCalledOnUpdateFrameReset() throws {
            struct TestRule: Rule {
                @Attribute var input: Int
                var value: Int { input + 1 }
            }

            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let (input, attribute) = subgraph.apply {
                let input = Attribute(value: 1)
                let attribute = Attribute(TestRule(input: input))
                return (input, attribute)
            }

            let _ = attribute.value
            input.value = 2
            try #require(recorder.history.nodeSetEdgePendingEntries.count == 3)

            let _ = attribute.value

            let nodeSetEdgePendingEntries = Array(recorder.history.nodeSetEdgePendingEntries)
            try #require(nodeSetEdgePendingEntries.count == 4)
            #expect(nodeSetEdgePendingEntries[3].attribute == attribute.identifier)
            #expect(nodeSetEdgePendingEntries[3].input == input.identifier)
            #expect(nodeSetEdgePendingEntries[3].pending == false)
        }
    }
    
    @Suite
    struct NodeSetDirtyTests {
        struct TestRule: Rule {
            var value: Int { 1 }
        }
        
        @Test
        func traceNodeSetDirtyCalledOnInvalidateInitializedAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule(), initialValue: 0)
            }

            try #require(recorder.history.nodeSetDirtyEntries.count == 0)
            
            attribute.invalidateValue()

            let nodeSetDirtyEntries = recorder.history.nodeSetDirtyEntries
            try #require(nodeSetDirtyEntries.count == 1)
            #expect(nodeSetDirtyEntries[0].attribute == attribute.identifier)
            #expect(nodeSetDirtyEntries[0].dirty == true)
        }
        
        @Test
        func traceNodeSetDirtyNotCalledOnInvalidateUninitializedAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule())
            }

            try #require(recorder.history.nodeSetDirtyEntries.count == 0)
            
            attribute.invalidateValue()

            let nodeSetDirtyEntries = recorder.history.nodeSetDirtyEntries
            try #require(nodeSetDirtyEntries.count == 0)
        }
        
        @Test
        func traceNodeSetDirtyNotCalledOnInvalidateExternalAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(value: 1)
            }

            try #require(recorder.history.nodeSetDirtyEntries.count == 0)
            
            attribute.invalidateValue()

            let nodeSetDirtyEntries = recorder.history.nodeSetDirtyEntries
            try #require(nodeSetDirtyEntries.count == 0)
        }
    }

    @Suite
    struct NodeSetPendingTests {
        struct TestRule: Rule {
            var value: Int { 1 }
        }

        @Test
        func traceNodeSetPendingCalledOnInvalidateInitializedAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule(), initialValue: 0)
            }

            try #require(recorder.history.nodeSetPendingEntries.count == 0)

            attribute.invalidateValue()

            let nodeSetPendingEntries = recorder.history.nodeSetPendingEntries
            try #require(nodeSetPendingEntries.count == 1)
            #expect(nodeSetPendingEntries[0].attribute == attribute.identifier)
            #expect(nodeSetPendingEntries[0].pending == true)
        }
        
        @Test
        func traceNodeSetPendingNotCalledOnInvalidateUninitializedAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule())
            }

            try #require(recorder.history.nodeSetPendingEntries.count == 0)

            attribute.invalidateValue()

            let nodeSetPendingEntries = recorder.history.nodeSetPendingEntries
            try #require(nodeSetPendingEntries.count == 0)
        }
        
        @Test
        func traceNodeSetPendingNotCalledOnInvalidateExternalAttribute() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(value: 1)
            }

            try #require(recorder.history.nodeSetPendingEntries.count == 0)

            attribute.invalidateValue()

            let nodeSetPendingEntries = recorder.history.nodeSetPendingEntries
            try #require(nodeSetPendingEntries.count == 0)
        }
    }

    @Suite
    struct NodeSetValueTests {
        struct TestRule: Rule {
            var value: Int { 1 }
        }
        
        @Test
        func traceNodeSetValueCalledOnCreateAttributeWithValue() throws {
            class NodeSetValueTrace: TestTraceRecorder {
                var capturedValue: Int?

                override func nodeSetValue(attribute: AnyAttribute, value: UnsafeRawPointer) {
                    super.nodeSetValue(attribute: attribute, value: value)
                    capturedValue = value.assumingMemoryBound(to: Int.self).pointee
                }
            }

            let graph = Graph()
            let recorder = NodeSetValueTrace()
            recorder.install(graph: graph)
            
            try #require(recorder.history.nodeSetValueEntries.count == 0)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(value: 42)
            }
            
            let nodeSetValueEntries = recorder.history.nodeSetValueEntries
            try #require(nodeSetValueEntries.count == 1)
            #expect(nodeSetValueEntries[0].attribute == attribute.identifier)
            // nodeSetValueEntries[0].value is not valid by this point
            #expect(recorder.capturedValue == 42)
        }
        
        @Test
        func traceNodeSetValueNotCalledOnCreateAttributeWithType() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            try #require(recorder.history.nodeSetValueEntries.count == 0)
            
            let subgraph = Subgraph(graph: graph)
            let _ = subgraph.apply {
                Attribute(type: Int.self)
            }
            
            let nodeSetValueEntries = recorder.history.nodeSetValueEntries
            try #require(nodeSetValueEntries.count == 0)
        }
        
        @Test
        func traceNodeSetValueCalledOnCreateAttributeWithRuleAndInitialValue() throws {
            class NodeSetValueTrace: TestTraceRecorder {
                var capturedValue: Int?

                override func nodeSetValue(attribute: AnyAttribute, value: UnsafeRawPointer) {
                    super.nodeSetValue(attribute: attribute, value: value)
                    capturedValue = value.assumingMemoryBound(to: Int.self).pointee
                }
            }

            let graph = Graph()
            let recorder = NodeSetValueTrace()
            recorder.install(graph: graph)
            
            try #require(recorder.history.nodeSetValueEntries.count == 0)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule(), initialValue: 42)
            }
            
            let nodeSetValueEntries = recorder.history.nodeSetValueEntries
            try #require(nodeSetValueEntries.count == 1)
            #expect(nodeSetValueEntries[0].attribute == attribute.identifier)
            // nodeSetValueEntries[0].value is not valid by this point
            #expect(recorder.capturedValue == 42)
        }
        
        @Test
        func traceNodeSetValueCalledOnCreateUnitializedAttributeWithRule() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            try #require(recorder.history.nodeSetValueEntries.count == 0)
            
            let subgraph = Subgraph(graph: graph)
            let _ = subgraph.apply {
                Attribute(TestRule())
            }
            
            let nodeSetValueEntries = recorder.history.nodeSetValueEntries
            try #require(nodeSetValueEntries.count == 0)
        }
        
        @Test
        func traceNodeSetValueCalledOnSetValue() throws {
            class NodeSetValueTrace: TestTraceRecorder {
                var capturedValue: Int?

                override func nodeSetValue(attribute: AnyAttribute, value: UnsafeRawPointer) {
                    super.nodeSetValue(attribute: attribute, value: value)
                    capturedValue = value.assumingMemoryBound(to: Int.self).pointee
                }
            }

            let graph = Graph()
            let recorder = NodeSetValueTrace()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(value: 0)
            }
            
            try #require(recorder.history.nodeSetValueEntries.count == 1)

            attribute.value = 42

            let nodeSetValueEntries = recorder.history.nodeSetValueEntries
            try #require(nodeSetValueEntries.count == 2)
            #expect(nodeSetValueEntries[0].attribute == attribute.identifier)
            #expect(recorder.capturedValue == 42)
        }
    }

    @Suite
    struct NodeMarkValueTests {
        @Test
        func traceNodeMarkValueCalledOnInvalidateValue() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(value: 0)
            }
            
            try #require(recorder.history.nodeMarkValueEntries.count == 0)

            attribute.invalidateValue()

            let nodeMarkValueEntries = recorder.history.nodeMarkValueEntries
            try #require(nodeMarkValueEntries.count == 1)
            #expect(nodeMarkValueEntries[0].attribute == attribute.identifier)
        }
    }

    @Suite
    struct BeginModifyNodeTests {
        struct TestRule: Rule {
            var flag: Int
            var value: Int {
                flag
            }
        }

        @Test
        func traceBeginModifyNodeCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule(flag: 1))
            }

            try #require(recorder.history.beginModifyNodeEntries.count == 0)

            attribute.mutateBody(as: TestRule.self, invalidating: false) { body in
                body.flag = 2
            }

            let beginModifyNodeEntries = recorder.history.beginModifyNodeEntries
            try #require(beginModifyNodeEntries.count == 1)
            #expect(beginModifyNodeEntries[0].attribute == attribute.identifier)
        }
    }

    @Suite
    struct EndModifyNodeTests {
        struct TestRule: Rule {
            var flag: Int
            var value: Int {
                flag
            }
        }

        @Test
        func traceEndModifyNodeCalled() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)
            
            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(TestRule(flag: 1))
            }

            try #require(recorder.history.endModifyNodeEntries.count == 0)

            attribute.mutateBody(as: TestRule.self, invalidating: false) { body in
                body.flag = 2
            }

            let endModifyNodeEntries = recorder.history.endModifyNodeEntries
            try #require(endModifyNodeEntries.count == 1)
            #expect(endModifyNodeEntries[0].attribute == attribute.identifier)
        }
    }
}
