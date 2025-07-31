public struct RuleContext<Value> {

    public var attribute: Attribute<Value>

    public init(attribute: Attribute<Value>) {
        self.attribute = attribute
    }
    
    @_extern(c, "AGGraphWithUpdate")
    private static func withUpdate(_ attribute: AnyAttribute, body: @escaping () -> Void)

    public func update(body: @escaping () -> Void) {
        RuleContext.withUpdate(attribute.identifier, body: body)
    }

    public var value: Value {
        unsafeAddress {
            return UnsafePointer(__AGGraphGetOutputValue(Metadata(Value.self))!.assumingMemoryBound(to: Value.self))
        }
        nonmutating set {
            withUnsafePointer(to: newValue) { newValuePointer in
                Graph.setOutputValue(newValuePointer)
            }
        }
    }

    public var hasValue: Bool {
        let valuePointer = __AGGraphGetOutputValue(Metadata(Value.self))
        return valuePointer != nil
    }

    public func changedValue<T>(of input: Attribute<T>, options: AGValueOptions) -> (value: T, changed: Bool) {
        let value = __AGGraphGetInputValue(
            attribute.identifier,
            input.identifier,
            options,
            Metadata(T.self)
        )
        return (
            value.value.assumingMemoryBound(to: T.self).pointee,
            value.flags.contains(.changed)
        )
    }

    public func valueAndFlags<InputValue>(of input: Attribute<InputValue>, options: AGValueOptions) -> (
        value: InputValue, flags: AGChangedValueFlags
    ) {
        let value = __AGGraphGetInputValue(
            attribute.identifier,
            input.identifier,
            options,
            Metadata(InputValue.self)
        )
        return (
            value.value.assumingMemoryBound(to: InputValue.self).pointee,
            value.flags.contains(.changed) ? .changed : []
        )
    }

    public subscript<InputValue>(_ input: Attribute<InputValue>) -> InputValue {
        unsafeAddress {
            return __AGGraphGetInputValue(attribute.identifier, input.identifier, [], Metadata(InputValue.self))
                .value
                .assumingMemoryBound(to: InputValue.self)
        }
    }

    public subscript<T>(_ weakInput: WeakAttribute<T>) -> T? {
        return weakInput.attribute.map { input in
            return self[input]
        }
    }

    public subscript<T>(_ optionalInput: OptionalAttribute<T>) -> T? {
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
