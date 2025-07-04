public protocol ObservedAttribute: _AttributeBody {

    func destroy()
    
}

extension ObservedAttribute {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {
        self.assumingMemoryBound(to: Self.self).pointee.destroy()
    }

    public static var _hasDestroySelf: Bool {
        return true
    }

}
