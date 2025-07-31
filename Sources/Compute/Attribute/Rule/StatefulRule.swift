import ComputeCxx

public protocol StatefulRule: _AttributeBody {

    associatedtype Value

    static var initialValue: Value? { get }
    mutating func updateValue()

}

extension StatefulRule {

    public static var initialValue: Value? {
        return nil
    }

    public static func _updateDefault(_ default: UnsafeMutableRawPointer) {
        guard let initialValue = initialValue else {
            return
        }
        withUnsafePointer(to: initialValue) { initialValuePointer in
            __AGGraphSetOutputValue(initialValuePointer, Metadata(Value.self))
        }
    }
}

extension StatefulRule {

    public static func _update(_ self: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        let rule = self.assumingMemoryBound(to: Self.self)
        rule.pointee.updateValue()
    }

    public var bodyChanged: Bool {
        fatalError("not implemented")
    }

    public var value: Value {
        unsafeAddress {
            guard let result = __AGGraphGetOutputValue(Metadata(Value.self)) else {
                preconditionFailure()
            }
            let pointer = result.assumingMemoryBound(to: Value.self)
            return UnsafePointer(pointer)
        }
        nonmutating set {
            context.value = newValue
        }
    }

    public var hasValue: Bool {
        return context.hasValue
    }

    public var attribute: Attribute<Value> {
        return Attribute<Value>(identifier: AnyAttribute.current!)
    }
    
    public var context: RuleContext<Value> {
        return RuleContext<Value>(attribute: attribute)
    }

}
