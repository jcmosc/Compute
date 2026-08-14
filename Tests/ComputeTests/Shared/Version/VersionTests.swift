import Testing

@Suite
struct VersionTests {
    @Test
    func versionConstant() {
        #if COMPATIBILITY_TESTS
        if #available(macOS 26, iOS 26, tvOS 26, watchOS 26, *) {
            #expect(version == 0x20021)
        } else {
            // not tested yet
        }
        #else
        #if CompatibilityModeAttributeGraphV6
        #expect(version == 0x2001e)
        #else
        #expect(version == 0x20021)
        #endif
        #endif
    }
}
