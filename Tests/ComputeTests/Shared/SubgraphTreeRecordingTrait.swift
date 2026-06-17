import Semaphore
import Testing

public struct SubgraphTreeRecordingTrait: TestTrait, TestScoping, SuiteTrait {
    private static let semaphore = AsyncSemaphore(value: 1)

    @MainActor
    public func provideScope(
        for test: Test,
        testCase: Test.Case?,
        performing function: @Sendable () async throws -> Void
    ) async throws {
        await Self.semaphore.wait()
        defer { Self.semaphore.signal() }

        Subgraph.setShouldRecordTree()
        try await function()
    }

    public var isRecursive: Bool {
        true
    }

}

extension Trait where Self == SubgraphTreeRecordingTrait {

    public static var recordTree: Self {
        return SubgraphTreeRecordingTrait()
    }

}
