import ComputeCxx

public protocol _AttributeBody {

    static func _destroySelf(_ self: UnsafeMutableRawPointer)
    static var _hasDestroySelf: Bool { get }

    static func _updateDefault(_ default: UnsafeMutableRawPointer)

    static var comparisonMode: AGComparisonMode { get }
    static var flags: AGAttributeTypeFlags { get }

}

extension _AttributeBody {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {

    }

    public static var _hasDestroySelf: Bool {
        return false
    }

    public static func _updateDefault(_ default: UnsafeMutableRawPointer) {

    }

    public static var comparisonMode: AGComparisonMode {
        return .equatableUnlessPOD
    }

    public static var flags: AGAttributeTypeFlags {
        return .option8
    }

}

extension _AttributeBody {

    public var updateWasCancelled: Bool {
        return __AGGraphUpdateWasCancelled()
    }

}
