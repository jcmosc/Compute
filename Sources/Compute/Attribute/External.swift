import ComputeCxx

public struct _External {
    
    public init() {}

    public static func _update(_: UnsafeMutableRawPointer, attribute: AnyAttribute) {}
    
}

extension _External: CustomStringConvertible {

    public var description: String {
        return "_External"
    }

}

extension _External: _AttributeBody {
    public static var comparisonMode: ComparisonMode {
        return .equatableAlways
    }
    public static var flags: _AttributeType.Flags {
        return []
    }
}

public struct External<Value> {

    public init() {}

    public static func _update(_: UnsafeMutableRawPointer, attribute: AnyAttribute) {}

}

extension External: _AttributeBody {

    public static var comparisonMode: ComparisonMode {
        return .equatableAlways
    }

    public static var flags: _AttributeType.Flags {
        return []
    }

}

extension External: CustomStringConvertible {

    public var description: String {
        return Metadata(Value.self).description
    }

}
