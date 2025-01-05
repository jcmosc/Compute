public struct RuleContext<Value> {

    public var attribute: Attribute<Value>

    public init(attribute: Attribute<Value>) {
        self.attribute = attribute
    }

    public func update(body: () -> Void) {
        fatalError("not implemented")
    }

    public var value: Value {
        unsafeAddress {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public var hasValue: Bool {
        fatalError("not implemented")
    }

    public func changedValue<T>(of attribute: Attribute<T>, options: ValueOptions) -> (value: Value, changed: Bool) {
        fatalError("not implemented")
    }

    public func valueAndFlags<T>(of attribute: Attribute<T>, options: ValueOptions) -> (
        value: Value, flags: ChangedValueFlags
    ) {
        fatalError("not implemented")
    }

    public subscript<T>(_ attribute: Attribute<T>) -> T {
        unsafeAddress {
            fatalError("not implemented")
        }
    }

    public subscript<T>(_ weakAttribute: WeakAttribute<T>) -> T? {
        fatalError("not implemented")
    }

    public subscript<T>(_ optionalAttribute: OptionalAttribute<T>) -> T? {
        fatalError("not implemented")
    }

}

extension RuleContext: Equatable {

    public static func == (_ lhs: RuleContext, _ rhs: RuleContext) -> Bool {
        return lhs.attribute == rhs.attribute
    }

}
