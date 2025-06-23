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
            return Attribute(identifier:  identifier.indirectSource)
        }
        nonmutating set {
            identifier.indirectSource = newValue.identifier
        }
    }
    
    public var attribute: Attribute<Value> {
        return Attribute(identifier: identifier)
    }

    public func resetSource() {
        __AGGraphResetIndirectAttribute(identifier, false)
    }

    public var dependency: AnyAttribute? {
        get {
            return identifier.indirectDependency
        }
        nonmutating set {
            identifier.indirectDependency = newValue
        }
    }

    public var value: Value {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public func changedValue(options: ValueOptions) -> (value: Value, changed: Bool) {
        fatalError("not implemented")
    }

    public var wrappedValue: Value {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public var projectedValue: Attribute<Value> {
        fatalError("not implemented")
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        fatalError("not implemented")
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
