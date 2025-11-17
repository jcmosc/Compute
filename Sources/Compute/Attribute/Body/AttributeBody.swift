import ComputeCxx

public protocol _AttributeBody {

    static func _destroySelf(_ self: UnsafeMutableRawPointer)
    static var _hasDestroySelf: Bool { get }

    static func _updateDefault(_ default: UnsafeMutableRawPointer)

    static var comparisonMode: ComparisonMode { get }
    static var flags: _AttributeType.Flags { get }

}

extension _AttributeBody {

    public static func _destroySelf(_ self: UnsafeMutableRawPointer) {}

    public static var _hasDestroySelf: Bool {
        return false
    }

    public static func _updateDefault(_ default: UnsafeMutableRawPointer) {}

    public static var comparisonMode: ComparisonMode {
        return .equatableUnlessPOD
    }

    public static var flags: _AttributeType.Flags {
        return .mainThread
    }

}

extension _AttributeBody {

    public var updateWasCancelled: Bool {
        return Graph.updateWasCancelled
    }

}
