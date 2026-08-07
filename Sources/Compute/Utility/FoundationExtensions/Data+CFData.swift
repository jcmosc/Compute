import Foundation

extension Data {
    @usableFromInline
    var cfData: CFData {
        #if canImport(Darwin)
        self as CFData
        #else
        withUnsafeBytes { raw -> CFData in
            CFDataCreate(
                kCFAllocatorDefault,
                raw.baseAddress?.assumingMemoryBound(to: UInt8.self),
                raw.count
            )!
        }
        #endif
    }
}
