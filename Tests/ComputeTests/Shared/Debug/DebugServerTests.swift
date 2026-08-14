#if os(Darwin)
import Foundation
import Testing

#if COMPATIBILITY_TESTS
// os_variant_has_internal_diagnostics("com.apple.AttributeGraph") returns false
let hasInternalDiagnostics = false
#else
let hasInternalDiagnostics = true
#endif

@Suite(.enabled(if: hasInternalDiagnostics), .serialized(for: \DebugServer.self))
struct DebugServerTests {
    @Test
    func urlOnlyAvailableWhileServerEnabled() throws {
        DebugServer.start(options: [.enabled])

        let url = try #require(DebugServer.url)
        #expect(try /graph:\/\/127\.0\.0\.1:\d+\/\?token=\d+/.wholeMatch(in: "\(url)") != nil)

        DebugServer.stop()
        #expect(DebugServer.url == nil)
    }

    @Test
    @available(macOS 26, iOS 26, tvOS 26, watchOS 26, *)
    func debugServer() async throws {
        DebugServer.start(options: [DebugServer.Options.enabled])
        defer {
            DebugServer.stop()
        }

        let url = try #require(DebugServer.url)
        let client = try #require(DebugClient(url: url as URL))

        let response = try await client.performCommand("graph/description") as? NSDictionary
        #expect(
            response == [
                "version": 2,
                "counters": [
                    "bytes": 0,
                    "max_bytes": 0,
                ],
                "graphs": [],
            ]
        )
    }
}

#endif
