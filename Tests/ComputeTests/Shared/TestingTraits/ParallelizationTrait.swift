import Synchronization
import Testing

public struct ParallelizationTrait: TestTrait, SuiteTrait {
    public struct Dependency: Sendable, Hashable {
        nonisolated(unsafe) let keyPath: AnyKeyPath
    }

    /// This instance's dependency.
    var dependency: Dependency

    /// A mapping of dependencies to serializers.
    private static let serializers = Mutex<[Dependency: Serializer]>([:])
}

extension ParallelizationTrait {
    public var isRecursive: Bool {
        true
    }

    public func prepare(for test: Test) async throws {
        // Ensure a serializer has been created for this trait's dependency
        Self.serializers.withLock { serializers in
            if serializers[dependency] == nil {
                serializers[dependency] = Serializer()
            }
        }
    }
}

extension ParallelizationTrait: TestScoping {
    public func provideScope(
        for test: Test,
        testCase: Test.Case?,
        performing function: @Sendable () async throws -> Void
    ) async throws {
        if test.isSuite {
            // Suites do not need to use a serializer since they don't run their own
            // code. Test functions within the suite will use serializers as needed.
            return try await function()
        }

        // Fetch the serializer for that dependency and run the test in serial with
        // any other tests that have the same dependency.
        let serializer = Self.serializers.withLock { serializers in
            guard let serializer = serializers[dependency] else {
                fatalError("Failed to find serializer for serialization trait '\(self)'.")
            }
            return serializer
        }
        try await serializer.run {
            try await function()
        }
    }
}

extension Trait where Self == ParallelizationTrait {
    public static func serialized(for keyPath: AnyKeyPath) -> Self {
        Self(dependency: ParallelizationTrait.Dependency(keyPath: keyPath))
    }
}

// MARK: - CustomStringConvertible

extension ParallelizationTrait: CustomStringConvertible {
    public var description: String {
        ".serialized(for: \(dependency))"
    }
}
