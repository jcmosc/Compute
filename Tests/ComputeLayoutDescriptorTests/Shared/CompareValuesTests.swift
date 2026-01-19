import Testing

/// A type that can be equal without being bitwise identical.
struct Parity: Equatable, RawRepresentable {
    var rawValue: Int

    var isEven: Bool {
        return rawValue % 2 == 0
    }

    static func == (lhs: Parity, rhs: Parity) -> Bool {
        return lhs.isEven == rhs.isEven
    }
}

struct WithExistential {
    var type: Any.Type
    var int: Int
}

enum InlineEnum<Value> {
    case empty
    case node(value: Value)
}

extension InlineEnum: Equatable where Value: Equatable {
    static func == (a: InlineEnum<Value>, b: InlineEnum<Value>) -> Bool {
        switch (a, b) {
        case (.empty, .empty):
            return true
        case (.node(let valueA), .node(let valueB)):
            return valueA == valueB
        default:
            return false
        }
    }
}

enum RecursiveEnum<Value> {
    case empty
    indirect case node(value: Value, next: RecursiveEnum<Value>)
}

extension RecursiveEnum: Equatable where Value: Equatable {
    static func == (a: RecursiveEnum<Value>, b: RecursiveEnum<Value>) -> Bool {
        switch (a, b) {
        case (.empty, .empty):
            return true
        case (.node(let valueA, let nextA), .node(let valueB, let nextB)):
            return valueA == valueB && nextA == nextB
        default:
            return false
        }
    }
}

@Suite
struct CompareValuesTests {

    @Test
    func compareInlineEnumValues() async {
        await #expect(processExitsWith: .success) {
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            #expect(
                compareValues(
                    InlineEnum<Parity>.empty,
                    InlineEnum<Parity>.empty
                ) == true
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.empty
                ) == false
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.empty,
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0))
                ) == false
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0))
                ) == true
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 1))
                ) == false
            )

            // default mode is .equatableAlways
            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 2))
                ) == true
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 2)),
                    mode: .bitwise
                ) == false
            )

            // POD because case is inline
            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 2)),
                    mode: .equatableUnlessPOD
                ) == false
            )

            #expect(
                compareValues(
                    InlineEnum<Parity>.node(value: Parity(rawValue: 0)),
                    InlineEnum<Parity>.node(value: Parity(rawValue: 2)),
                    mode: .equatableAlways
                ) == true
            )
        }
    }

    @Test
    func compareRecursiveEnumValues() async {
        await #expect(processExitsWith: .success) {
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.empty,
                    RecursiveEnum<Parity>.empty
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.empty
                ) == false
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.empty,
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty)
                ) == false
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty)
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 1), next: .empty)
                ) == false
            )

            // default mode is .equatableAlways
            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 2), next: .empty)
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 2), next: .empty),
                    mode: .bitwise
                ) == false
            )

            // not POD because case is indirect
            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 2), next: .empty),
                    mode: .equatableUnlessPOD
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 0), next: .empty),
                    RecursiveEnum<Parity>.node(value: Parity(rawValue: 2), next: .empty),
                    mode: .equatableAlways
                ) == true
            )
        }
    }

    @Test
    func compareRecursiveEnumNonEquatableValues() async {
        await #expect(processExitsWith: .success) {
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.empty,
                    RecursiveEnum<WithExistential>.empty
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty),
                    RecursiveEnum<WithExistential>.empty
                ) == false
            )

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.empty,
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty)
                ) == false
            )

            #expect(
                compareValues(
                    RecursiveEnum<Int>.node(value: 62, next: .empty),
                    RecursiveEnum<Int>.node(value: 62, next: .empty)
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty),
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty)
                ) == true
            )

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty),
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 1), next: .empty)
                ) == false
            )

            #expect(
                compareValues(
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: Int.self, int: 0), next: .empty),
                    RecursiveEnum<WithExistential>.node(value: WithExistential(type: String.self, int: 0), next: .empty)
                ) == false
            )
        }
    }

}
