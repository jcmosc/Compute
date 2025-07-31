import ComputeCxx

public protocol Rule: _AttributeBody {

    associatedtype Value

    static var initialValue: Value? { get }
    var value: Value { get }

}

extension Rule {

    public static var initialValue: Value? {
        return nil
    }

    public static func _updateDefault(_ self: UnsafeMutableRawPointer) {
        guard let initialValue = initialValue else {
            return
        }
        withUnsafePointer(to: initialValue) { initialValuePointer in
            Graph.setOutputValue(initialValuePointer)
        }

    }

    public static func _update(_ self: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        let rule = self.assumingMemoryBound(to: Self.self)
        let value = rule.pointee.value
        withUnsafePointer(to: value) { valuePointer in
            Graph.setOutputValue(valuePointer)
        }
    }

}

extension Rule {

    public var bodyChanged: Bool {
        fatalError("not implemented")
    }

    public var attribute: Attribute<Value> {
        return Attribute<Value>(identifier: AnyAttribute.current!)
    }

    public var context: RuleContext<Value> {
        return RuleContext<Value>(attribute: attribute)
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
        options: CachedValueOptions,
        owner: AnyAttribute?,
        hashValue: Int,
        bodyPtr: UnsafeRawPointer,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UnsafePointer<Value> {
        fatalError("not implemented")
    }

}
