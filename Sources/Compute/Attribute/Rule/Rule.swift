import ComputeCxx

public protocol Rule: _AttributeBody {

    associatedtype Value

    var value: Value { get }
    static var initialValue: Value? { get }

}

extension Rule {

    public static var initialValue: Value? {
        return nil
    }

    public static func _updateDefault(_ self: UnsafeMutableRawPointer) {
        fatalError("not implemented")

    }

    public static func _update(_ self: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        fatalError("not implemented")
    }

}

extension Rule {

    public var bodyChanged: Bool {
        fatalError("not implemented")
    }

    public var attribute: Attribute<Value> {
        fatalError("not implemented")
    }

    public var context: RuleContext<Value> {
        fatalError("not implemented")
    }

}

public struct CachedValueOptions {}

extension Rule where Value: Hashable {

    public func cachedValue(options: CachedValueOptions, owner: AnyAttribute?) -> Value {
        fatalError("not implemented")
    }

    public func cachedValueIfExists(options: CachedValueOptions, owner: AnyAttribute?) -> Value? {
        fatalError("not implemented")
    }

    public static func _cachedValue(
        options: CachedValueOptions, owner: AnyAttribute?, hashValue: Int, bodyPtr: UnsafeRawPointer,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UnsafePointer<Value> {
        fatalError("not implemented")
    }

}
