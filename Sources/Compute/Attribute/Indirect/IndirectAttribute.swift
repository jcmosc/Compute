import ComputeCxx

@propertyWrapper
@dynamicMemberLookup
public struct IndirectAttribute<Value> {

    public var identifier: AnyAttribute

    public init(source: Attribute<Value>) {
        identifier = __AGGraphCreateIndirectAttribute2(source.identifier, MemoryLayout<Value>.size)
    }

    public var source: Attribute<Value> {
        get {
            return Attribute(identifier: __AGGraphGetIndirectAttribute(identifier))
        }
        set {
            __AGGraphSetIndirectAttribute(identifier, newValue.identifier)
        }
    }

    public func resetSource() {
        __AGGraphResetIndirectAttribute(identifier, false)
    }

    public var attribute: Attribute<Value> {
        return Attribute(identifier: identifier)
    }

    public var dependency: AnyAttribute? {
        get {
            let result = __AGGraphGetIndirectDependency(identifier)
            return result == .nil ? nil : result
        }
        set {
            __AGGraphSetIndirectDependency(identifier, newValue ?? .nil)
        }
    }
    
    public var value: Value {
        get {
            return Attribute(identifier: identifier).value
        }
        nonmutating set {
            Attribute(identifier: identifier).value = newValue
        }
        nonmutating _modify {
            yield &Attribute<Value>(identifier: identifier).value
        }
    }

    public func changedValue(options: AGValueOptions) -> (value: Value, changed: Bool) {
        return Attribute(identifier: identifier).changedValue(options: options)
    }
    
    public var wrappedValue: Value {
        get {
            value
        }
        nonmutating set {
            value = newValue
        }
        nonmutating _modify {
            yield &value
        }
    }

    

    public var projectedValue: Attribute<Value> {
        return Attribute(identifier: identifier)
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        return Attribute(identifier: identifier)[dynamicMember: keyPath]
    }

}

extension IndirectAttribute: Equatable {

    public static func == (_ lhs: IndirectAttribute<Value>, _ rhs: IndirectAttribute<Value>) -> Bool {
        return lhs.identifier == rhs.identifier
    }

}

extension IndirectAttribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(identifier)
    }

}
