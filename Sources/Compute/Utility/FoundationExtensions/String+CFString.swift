import Foundation

extension String {
    init(_ cfString: CFString) {
        if let ptr = CFStringGetCStringPtr(cfString, CFStringBuiltInEncodings.UTF8.rawValue) {
            self.init(cString: ptr)
            return
        }

        let length = CFStringGetLength(cfString)
        let maxSize = CFStringGetMaximumSizeForEncoding(length, CFStringBuiltInEncodings.UTF8.rawValue) + 1

        let buffer = UnsafeMutablePointer<CChar>.allocate(capacity: maxSize)
        defer { buffer.deallocate() }

        guard CFStringGetCString(cfString, buffer, maxSize, CFStringBuiltInEncodings.UTF8.rawValue) else {
            self = ""
            return
        }
        self.init(cString: buffer)
    }

    var cfString: CFString {
        guard let cString = self.cString(using: .utf8),
            let cfString = CFStringCreateWithCString(nil, cString, CFStringBuiltInEncodings.UTF8.rawValue)
        else {
            let utf8 = Array(self.utf8)
            return CFStringCreateWithBytes(nil, utf8, utf8.count, CFStringBuiltInEncodings.UTF8.rawValue, false)
        }
        return cfString
    }

}
