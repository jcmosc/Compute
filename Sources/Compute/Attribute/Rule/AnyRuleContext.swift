import ComputeCxx

public struct AnyRuleContext {

    public var attribute: AnyAttribute

    public init(attribute: AnyAttribute) {
        self.attribute = attribute
    }

    public init<Value>(_ ruleContext: RuleContext<Value>) {
        self.attribute = ruleContext.attribute.identifier
    }

    public func update(body: () -> Void) {
        struct Context {
            let body: () -> Void
        }
        // TODO: use silgen?
        withoutActuallyEscaping(body) { escapingBody in
            withUnsafePointer(to: Context(body: escapingBody)) { contextPointer in
                __AGGraphWithUpdate(
                    attribute,
                    {
                        $0.assumingMemoryBound(to: Context.self).pointee.body()
                    },
                    contextPointer
                )
            }
        }
    }

    public func changedValue<Value>(of input: Attribute<Value>, options: AGValueOptions) -> (
        value: Value, changed: Bool
    ) {
        let result = __AGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.changed
        )
    }

    public func valueAndFlags<Value>(of input: Attribute<Value>, options: AGValueOptions) -> (
        value: Value, flags: AGChangedValueFlags
    ) {
        let result = __AGGraphGetInputValue(attribute, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.changed ? .changed : []
        )
    }

    public subscript<Value>(_ input: Attribute<Value>) -> Value {
        unsafeAddress {
            return __AGGraphGetInputValue(attribute, input.identifier, [], Metadata(Value.self))
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

    public func unsafeCast<Value>(to type: Value.Type) -> RuleContext<Value> {
        return RuleContext<Value>(attribute: Attribute(identifier: attribute))
    }

}

extension AnyRuleContext: Equatable {

    public static func == (_ lhs: AnyRuleContext, _ rhs: AnyRuleContext) -> Bool {
        return lhs.attribute == rhs.attribute
    }

}
