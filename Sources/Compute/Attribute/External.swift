import ComputeCxx

public struct External<Value> {

    public init() {}

    public static func _update(_: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        fatalError("not implemented")
    }

}

extension External: _AttributeBody {

    public static var comparisonMode: AGComparisonMode {
        fatalError("not implemented")
    }

    public static var flags: AttributeTypeFlags {
        fatalError("not implemented")
    }

}

extension External: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}
