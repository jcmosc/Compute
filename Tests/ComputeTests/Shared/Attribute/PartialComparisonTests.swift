import Foundation
import Testing

@Suite(.serialized(for: \Subgraph.Type.current))
struct PartialComparisonTests {
    struct EnvironmentValues<Value> {
        var value: Value
    }

    enum Content<Value> {
        case keyPath(KeyPath<EnvironmentValues<Value>, Value>)
        case value(Value)
    }

    struct Container<Value> {
        var prefix: Int
        var content: Content<Value>
        var suffix: Int
    }

    struct ReadContent<Input>: StatefulRule {
        @Attribute var content: Content<Input>
        var environment: EnvironmentValues<Input>
        var updates = 0

        mutating func updateValue() {
            updates += 1
            let input: Input
            switch content {
            case let .keyPath(keyPath):
                input = environment[keyPath: keyPath]
            case let .value(value):
                input = value
            }
            value = (input, updates)
        }

        typealias Value = (input: Input, updates: Int)
    }

    @Test
    func enumMemberAfterPrefix() async throws {
        try await #require(processExitsWith: .success) {
            setenv(asyncLayoutsEnvironmentVariable, "0", 1)
            _ = prefetchCompareValues(
                type: Metadata(Container<Bool>.self),
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            withGraph {
                let source = Attribute(value: Container(
                    prefix: 0, content: .keyPath(\EnvironmentValues<Bool>.value), suffix: 0
                ))
                let result = Attribute(ReadContent(
                    content: source[offset: { .of(&$0.content) }], environment: .init(value: true)
                ))
                #expect(result.value.input)
                #expect(result.value.updates == 1)

                source.value.prefix = 1
                #expect(result.value.input)
                #expect(result.value.updates == 1)

                source.value.content = .value(false)
                #expect(!result.value.input)
                #expect(result.value.updates == 2)
            }
        }
    }

    struct Payload: Equatable {
        var first: Int
        var second: Int
    }

    struct ReadValue<Input>: StatefulRule {
        @Attribute var input: Input
        var updates = 0

        mutating func updateValue() {
            updates += 1
            value = (input, updates)
        }

        typealias Value = (input: Input, updates: Int)
    }

    struct Parity: Equatable {
        var rawValue: Int

        static func == (lhs: Self, rhs: Self) -> Bool {
            lhs.rawValue % 2 == rhs.rawValue % 2
        }
    }

    @Test
    func memberUsesItsEqualityImplementation() async throws {
        try await #require(processExitsWith: .success) {
            setenv(asyncLayoutsEnvironmentVariable, "0", 1)
            _ = prefetchCompareValues(
                type: Metadata(Triple<Int, Parity, Int>.self),
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            withGraph {
                let source = Attribute(value: Triple(first: 0, second: Parity(rawValue: 0), third: 0))
                let result = Attribute(ReadValue(input: source[offset: { .of(&$0.second) }]))
                #expect(result.value.input.rawValue == 0)
                #expect(result.value.updates == 1)

                source.value = Triple(first: 1, second: Parity(rawValue: 2), third: 0)
                #expect(result.value.updates == 1)

                source.value = Triple(first: 1, second: Parity(rawValue: 3), third: 0)
                #expect(result.value.input.rawValue == 3)
                #expect(result.value.updates == 2)
            }
        }
    }

    struct InlineValues: Equatable {
        var first: Int
        var second: Int
        var third: Int
    }

    struct ValueContainer {
        var values: InlineValues
        var content: Content<Bool>
    }

    @Test
    func byteComparisonStaysWithinTheMember() async throws {
        try await #require(processExitsWith: .success) {
            setenv(asyncLayoutsEnvironmentVariable, "0", 1)
            _ = prefetchCompareValues(
                type: Metadata(ValueContainer.self),
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            withGraph {
                let source = Attribute(value: ValueContainer(
                    values: InlineValues(first: 0, second: 1, third: 2), content: .value(true)
                ))
                let result = Attribute(ReadValue(input: source[offset: { .of(&$0.values.second) }]))
                #expect(result.value.input == 1)
                #expect(result.value.updates == 1)

                source.value.values.third = 3
                #expect(result.value.input == 1)
                #expect(result.value.updates == 1)

                source.value.values.second = 4
                #expect(result.value.input == 4)
                #expect(result.value.updates == 2)
            }
        }
    }

    @Test
    func enumPayloadIsNotCopiedFromAnAdjacentField() async throws {
        try await #require(processExitsWith: .success) {
            setenv(asyncLayoutsEnvironmentVariable, "0", 1)
            _ = prefetchCompareValues(
                type: Metadata(Container<Payload>.self),
                options: [.comparisonModeEquatableAlways, .fetchLayoutsSynchronously],
                priority: 0
            )

            withGraph {
                let payload = Payload(first: 0, second: 0x1_0000_0000)
                let source = Attribute(value: Container(prefix: 0, content: .value(payload), suffix: 0))
                let result = Attribute(ReadContent(
                    content: source[offset: { .of(&$0.content) }], environment: .init(value: payload)
                ))
                #expect(result.value.input == payload)
                #expect(result.value.updates == 1)

                source.value.prefix = 1
                #expect(result.value.input == payload)
                #expect(result.value.updates == 1)

                let changed = Payload(first: 1, second: payload.second)
                source.value.content = .value(changed)
                #expect(result.value.input == changed)
                #expect(result.value.updates == 2)
            }
        }
    }
}
