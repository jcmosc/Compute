import Semaphore
import Testing

public struct SubgraphApplyingTrait: TestTrait, TestScoping, SuiteTrait {
    nonisolated(unsafe) private static let sharedGraph = Graph()
    private static let semaphore = AsyncSemaphore(value: 1)

    @MainActor
    public func provideScope(
        for test: Test,
        testCase: Test.Case?,
        performing function: @Sendable () async throws -> Void
    ) async throws {
        await Self.semaphore.wait()
        defer { Self.semaphore.signal() }

        let graph = Graph(shared: Self.sharedGraph)
        let subgraph = Subgraph(graph: graph)
        let oldSubgraph = Subgraph.current

        // TODO: should Subgraph.apply be async?
        Subgraph.current = subgraph
        try await function()
        Subgraph.current = oldSubgraph
    }

    public var isRecursive: Bool {
        true
    }

}

extension Trait where Self == SubgraphApplyingTrait {

    public static var applySubgraph: Self {
        return SubgraphApplyingTrait()
    }

}
