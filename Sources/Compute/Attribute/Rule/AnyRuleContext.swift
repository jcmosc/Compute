public struct AnyRuleContext {

    public var attribute: AnyAttribute

    public init(attribute: AnyAttribute) {
        self.attribute = attribute
    }

    public init<Value>(_ ruleContext: RuleContext<Value>) {
        fatalError("not implemented")
    }

    public func unsafeCast<Value>(to type: Value.Type) -> RuleContext<Value> {
        fatalError("not implemented")
    }

    public func update(body: () -> Void) {
        fatalError("not implemented")
    }

    public func changedValue<Value>(of attribute: Attribute<Value>, options: ValueOptions) -> (
        value: Value, changed: Bool
    ) {
        fatalError("not implemented")
    }

    public func valueAndFlags<Value>(of attribute: Attribute<Value>, options: ValueOptions) -> (
        value: Value, flags: ChangedValueFlags
    ) {
        fatalError("not implemented")
    }

    public subscript<Value>(_ attribute: Attribute<Value>) -> Value {
        unsafeAddress {
            fatalError("not implemented")
        }
    }

    public subscript<Value>(_ weakAttribute: WeakAttribute<Value>) -> Value? {
        fatalError("not implemented")
    }

    public subscript<Value>(_ optionalAttribute: OptionalAttribute<Value>) -> Value? {
        fatalError("not implemented")
    }

}

extension AnyRuleContext: Equatable {

    public static func == (_ lhs: AnyRuleContext, _ rhs: AnyRuleContext) -> Bool {
        return lhs.attribute == rhs.attribute
    }

}
