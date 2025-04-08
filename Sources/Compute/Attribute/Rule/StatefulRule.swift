import ComputeCxx

public protocol StatefulRule: _AttributeBody {

    associatedtype Value

    static var initialValue: Value? { get }
    func updateValue()

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

    public static func _update(_ pointer: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        let rule = pointer.assumingMemoryBound(to: Self.self)
        rule.pointee.updateValue()
    }
}

extension StatefulRule {

    public var bodyChanged: Bool {
        // guessing this is it
        return __AGGraphCurrentAttributeWasModified()
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
        guard let attribute = AnyAttribute.current else {
            preconditionFailure()
        }
        return Attribute<Value>(identifier: attribute)
    }

    public var context: RuleContext<Value> {
        return RuleContext<Value>(attribute: attribute)
    }

}
