import ComputeCxx

public struct External<Value> {

    public init() {}

    public static func _update(_: UnsafeMutableRawPointer, attribute: AnyAttribute) {}

}

extension External: _AttributeBody {

    public static var comparisonMode: ComparisonMode {
        return .equatableAlways
    }

    public static var flags: Flags {
        return []
    }

}

extension External: CustomStringConvertible {

    public var description: String {
        return Metadata(Value.self).description
    }

}
