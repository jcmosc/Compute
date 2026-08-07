import Foundation

#if !canImport(Darwin)
extension Data {
    @usableFromInline
    var cfData: CFData {
        withUnsafeBytes { raw -> CFData in
            CFDataCreate(
                kCFAllocatorDefault,
                raw.baseAddress?.assumingMemoryBound(to: UInt8.self),
                raw.count
            )!
        }
        
    }
}
#endif
