public protocol ObservedAttribute: _AttributeBody {

    func destroy()
}

extension ObservedAttribute {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {
        fatalError("not implemented")
    }

    public static var _hasDestroySelf: Bool {
        fatalError("not implemented")
    }

}
