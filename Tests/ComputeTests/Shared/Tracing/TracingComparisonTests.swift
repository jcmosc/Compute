import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct TracingComparisonTests {
    @Suite
    struct CompareFailedTests {
        struct CounterRule: StatefulRule {
            var counter: Int = 0
            mutating func updateValue() {
                counter += 1
                context.value = CounterRule.Value(text: "value-\(counter)")
            }
            struct Value: Equatable {
                var text: String
            }
        }

        @Test
        func traceCompareFailedCalledOnValueChanged() throws {
            let graph = Graph()
            let recorder = TestTraceRecorder()
            recorder.install(graph: graph)

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                Attribute(CounterRule())
            }

            let firstValue = attribute.value
            #expect(firstValue == CounterRule.Value(text: "value-1"))

            // Initial value does not perform comparison
            try #require(recorder.history.compareFailedEntries.count == 0)

            attribute.invalidateValue()
            let secondValue = attribute.value
            #expect(secondValue == CounterRule.Value(text: "value-2"))

            // Subsequent value fails comparison
            let compareFailedEntries = recorder.history.compareFailedEntries
            try #require(compareFailedEntries.count == 1)
            #expect(compareFailedEntries[0].attribute == attribute.identifier)
        }
    }
}
