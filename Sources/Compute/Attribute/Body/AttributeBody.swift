import ComputeCxx

public protocol _AttributeBody {

    static func _destroySelf(_ self: UnsafeMutableRawPointer)
    static func _updateDefault(_ default: UnsafeMutableRawPointer)

    static var comparisonMode: AGComparisonMode { get }
    static var _hasDestroySelf: Bool { get }
    static var flags: AGAttributeTypeFlags { get }

}

extension _AttributeBody {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {
        fatalError("not implemented")
    }

    public static func _updateDefault(_ default: UnsafeMutableRawPointer) {
        fatalError("not implemented")
    }

    public static var comparisonMode: AGComparisonMode {
        fatalError("not implemented")
    }

    public static var _hasDestroySelf: Bool {
        fatalError("not implemented")
    }

    public static var flags: AGAttributeTypeFlags {
        fatalError("not implemented")
    }

}

extension _AttributeBody {

    public var updateWasCancelled: Bool {
        fatalError("not implemented")
    }

}
