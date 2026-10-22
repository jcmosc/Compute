import Foundation
import Testing

// This suite relies on process isolation, which does not complete on Linux.
#if !os(Linux)

private final class PendingBox {}

private struct PendingPayload {
    var name: String
    var box: PendingBox
}

/// The optional payload sits at offset 32.
private struct PendingInner {
    var a: Int
    var b: Int
    var c: Int
    var d: Int
    var payload: PendingPayload?
}

private struct PendingOuter {
    var tag: Int
    var inner: PendingInner
}

@Suite(.serialized)
struct PendingNestedLayoutTests {

    /// A layout built synchronously while a nested enum's layout is still queued visits the enum's
    /// cases inline, at a nonzero offset. The case contents must still be compared at their own offsets.
    @Test
    func enumCasesOfPendingNestedLayout() async {
        await #expect(processExitsWith: .success) {
            let mode = ComparisonOptions(mode: .equatableUnlessPOD)

            // Queue the optional's layout, leaving a placeholder in the layout cache. The background
            // queue runs at low priority, so the placeholder outlives the synchronous fetch below.
            _ = prefetchCompareValues(type: Metadata(PendingPayload?.self), options: mode, priority: 0)

            _ = prefetchCompareValues(
                type: Metadata(PendingOuter.self),
                options: ComparisonOptions(
                    rawValue: mode.rawValue | ComparisonOptions.fetchLayoutsSynchronously.rawValue
                ),
                priority: 0
            )

            let box = PendingBox()
            let lhs = PendingOuter(
                tag: 0,
                inner: PendingInner(a: 0, b: 0, c: 0, d: 0, payload: PendingPayload(name: "a", box: box))
            )
            let rhs = PendingOuter(
                tag: 0,
                inner: PendingInner(a: 0, b: 0, c: 0, d: 0, payload: PendingPayload(name: "b", box: box))
            )
            let equal = compareValues(
                lhs,
                rhs,
                options: ComparisonOptions(rawValue: mode.rawValue | ComparisonOptions.copyOnWrite.rawValue)
            )
            #expect(!equal, "expected values that differ in `payload.name` to compare unequal")
        }
    }
}

#endif
