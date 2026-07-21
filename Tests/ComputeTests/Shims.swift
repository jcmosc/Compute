@_exported public import Compute

let prefetchLayoutsEnvironmentVariable = "IAG_PREFETCH_LAYOUTS"
let asyncLayoutsEnvironmentVariable = "IAG_ASYNC_LAYOUTS"
let printLayoutsEnvironmentVariable = "IAG_PRINT_LAYOUTS"

extension Graph: @retroactive Equatable {
    public static func == (_ lhs: Graph, _ rhs: Graph) -> Bool {
        unsafeBitCast(lhs, to: UnsafeRawPointer.self) == unsafeBitCast(rhs, to: UnsafeRawPointer.self)
    }
}

extension Subgraph: @retroactive Equatable {
    public static func == (_ lhs: Subgraph, _ rhs: Subgraph) -> Bool {
        unsafeBitCast(lhs, to: UnsafeRawPointer.self) == unsafeBitCast(rhs, to: UnsafeRawPointer.self)
    }
}

#if os(Linux)
extension String {
    init?(cfString: CFString) {
        if let pointer = CFStringGetCStringPtr(cfString, CFStringBuiltInEncodings.UTF8.rawValue) {
            self.init(cString: pointer)
            return
        }

        let length = CFStringGetLength(cfString)
        let maxSize = CFStringGetMaximumSizeForEncoding(length, CFStringBuiltInEncodings.UTF8.rawValue) + 1
        let buffer = UnsafeMutablePointer<CChar>.allocate(capacity: maxSize)
        defer {
            buffer.deallocate()
        }
        guard CFStringGetCString(cfString, buffer, maxSize, CFStringBuiltInEncodings.UTF8.rawValue) else {
            return nil
        }
        self.init(cString: buffer)
    }
}

func autoreleasepool<E, Result>(invoking body: () throws(E) -> Result) throws(E) -> Result
where E: Error, Result: ~Copyable {
    try body()
}
#endif
