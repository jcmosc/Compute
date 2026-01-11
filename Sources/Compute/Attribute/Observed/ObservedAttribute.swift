public protocol ObservedAttribute: _AttributeBody {

    mutating func destroy()

}

extension ObservedAttribute {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {
        self.assumingMemoryBound(to: Self.self).pointee.destroy()
    }

    public static var _hasDestroySelf: Bool {
        return true
    }

}
