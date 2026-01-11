import ComputeCxx

public struct AnyRuleContext {

    public var attribute: AnyAttribute

    public init(attribute: AnyAttribute) {
        self.attribute = attribute
    }

    public init<Value>(_ ruleContext: RuleContext<Value>) {
        attribute = ruleContext.attribute.identifier
    }

    public func unsafeCast<Value>(to type: Value.Type) -> RuleContext<Value> {
        return RuleContext<Value>(attribute: Attribute(identifier: attribute))
    }

    @_extern(c, "AGGraphWithUpdate")
    private static func withUpdate(_ attribute: AnyAttribute, body: @escaping () -> Void)

    public func update(body: () -> Void) {
        // TODO: double check needs to be escaping
        withoutActuallyEscaping(body) { escapingBody in
            AnyRuleContext.withUpdate(attribute, body: escapingBody)
        }
    }

    public func changedValue<Value>(
        of input: Attribute<Value>,
        options: AGValueOptions
    ) -> (
        value: Value, changed: Bool
    ) {
        let result = __AGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.flags.contains(.changed)
        )
    }

    public func valueAndFlags<Value>(
        of input: Attribute<Value>,
        options: AGValueOptions
    ) -> (
        value: Value, flags: AGChangedValueFlags
    ) {
        let result = __AGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.flags.contains(.changed) ? .changed : []
        )
    }

    public subscript<Value>(_ attribute: Attribute<Value>) -> Value {
        unsafeAddress {
            return __AGGraphGetInputValue(self.attribute, attribute.identifier, [], Metadata(Value.self))
                .value
                .assumingMemoryBound(to: Value.self)
        }
    }

    public subscript<Value>(_ weakInput: WeakAttribute<Value>) -> Value? {
        return weakInput.attribute.map { input in
            return self[input]
        }
    }

    public subscript<Value>(_ optionalInput: OptionalAttribute<Value>) -> Value? {
        return optionalInput.attribute.map { input in
            return self[input]
        }
    }

}

extension AnyRuleContext: Equatable {

    public static func == (_ lhs: AnyRuleContext, _ rhs: AnyRuleContext) -> Bool {
        return lhs.attribute == rhs.attribute
    }

}
