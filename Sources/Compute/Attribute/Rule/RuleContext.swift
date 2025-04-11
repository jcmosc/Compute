public struct RuleContext<Value> {

    public var attribute: Attribute<Value>

    public init(attribute: Attribute<Value>) {
        self.attribute = attribute
    }

    public func update(body: () -> Void) {
        AnyRuleContext(attribute: attribute.identifier).update(body: body)
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
            withUnsafePointer(to: newValue) { newValuePointer in
                __AGGraphSetOutputValue(newValuePointer, Metadata(Value.self))
            }
        }
    }

    public var hasValue: Bool {
        let valuePointer = __AGGraphGetOutputValue(Metadata(Value.self))
        return valuePointer != nil
    }

    public func changedValue<T>(of input: Attribute<T>, options: AGValueOptions) -> (value: Value, changed: Bool) {
        let result = __AGGraphGetInputValue(attribute.identifier, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.changed
        )
    }

    public func valueAndFlags<T>(of input: Attribute<T>, options: AGValueOptions) -> (
        value: Value, flags: AGChangedValueFlags
    ) {
        let result = __AGGraphGetInputValue(attribute.identifier, input.identifier, options, Metadata(Value.self))
        return (
            result.value.assumingMemoryBound(to: Value.self).pointee,
            result.changed ? .changed : []
        )
    }

    public subscript<InputValue>(_ input: Attribute<InputValue>) -> InputValue {
        unsafeAddress {
            return __AGGraphGetInputValue(attribute.identifier, input.identifier, [], Metadata(InputValue.self))
                .value
                .assumingMemoryBound(to: InputValue.self)
        }
    }

    public subscript<InputValue>(_ weakInput: WeakAttribute<InputValue>) -> InputValue? {
        return weakInput.attribute.map { input in
            return self[input]
        }
    }

    public subscript<InputValue>(_ optionalInput: OptionalAttribute<InputValue>) -> InputValue? {
        return optionalInput.attribute.map { input in
            return self[input]
        }
    }

}

extension RuleContext: Equatable {

    public static func == (_ lhs: RuleContext, _ rhs: RuleContext) -> Bool {
        return lhs.attribute == rhs.attribute
    }

}
