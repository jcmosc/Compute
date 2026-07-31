import Semaphore

/// A type whose instances can run a series of work items in strict order.
final actor Serializer {
    private let semaphore = AsyncSemaphore(value: 1)

    /// Run a work item serially after any previously-scheduled work items.
    func run<R>(_ workItem: @isolated(any) @Sendable () async throws -> R) async rethrows -> R where R: Sendable {
        await semaphore.wait()
        defer { semaphore.signal() }

        return try await workItem()
    }
}
