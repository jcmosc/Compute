@propertyWrapper
@dynamicMemberLookup
public struct IndirectAttribute<Value> {

    public var identifier: AnyAttribute

    public init(source: Attribute<Value>) {
        fatalError("not implemented")
    }

    public var attribute: Attribute<Value> {
        fatalError("not implemented")
    }

    public var source: Attribute<Value> {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public func resetSource() {
        fatalError("not implemented")
    }

    public var dependency: AnyAttribute? {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
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
