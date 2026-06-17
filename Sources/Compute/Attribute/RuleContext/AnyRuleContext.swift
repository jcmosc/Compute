import ComputeCxx

@_silgen_name("IAGGraphWithUpdate")
func IAGGraphWithUpdate(_ attribute: AnyAttribute, body: () -> Void)

@frozen
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

    public func update(body: () -> Void) {
        IAGGraphWithUpdate(attribute, body: body)
    }

    public func changedValue<Value>(
        of input: Attribute<Value>,
        options: IAGValueOptions
    ) -> (
        value: Value, changed: Bool
    ) {
        let result = __IAGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.flags.contains(.changed)
        )
    }

    public func valueAndFlags<Value>(
        of input: Attribute<Value>,
        options: IAGValueOptions
    ) -> (
        value: Value, flags: IAGChangedValueFlags
    ) {
        let result = __IAGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.flags.contains(.changed) ? .changed : []
        )
    }

    public subscript<Value>(_ attribute: Attribute<Value>) -> Value {
        unsafeAddress {
            return __IAGGraphGetInputValue(self.attribute, attribute.identifier, [], Metadata(Value.self))
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
