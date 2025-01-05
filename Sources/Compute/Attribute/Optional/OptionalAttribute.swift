@propertyWrapper
@dynamicMemberLookup
public struct OptionalAttribute<Value> {
    
    public var base: AnyOptionalAttribute
    
    public init(base: AnyOptionalAttribute) {
        self.base = base
    }
    
    public init() {
        fatalError("not implemented")
    }
    
    public init(_ weakAttribute: WeakAttribute<Value>) {
        fatalError("not implemented")
    }
    
    public init(_ attribute: Attribute<Value>) {
        fatalError("not implemented")
    }
    
    public init(_ attribute: Attribute<Value>?) {
        fatalError("not implemented")
    }
    
    public var attribute: Attribute<Value>? {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }
    
    public var value: Value? {
        fatalError("not implemented")
    }

    public func changedValue() -> (value: Value, changed: Bool)? {
        fatalError("not implemented")
    }

    public func map<T>(_ transform: (Attribute<Value>) -> T) -> T? {
        fatalError("not implemented")
    }
    
    public var wrappedValue: Value? {
        fatalError("not implemented")
    }
    
    public var projectedValue: Attribute<Value>? {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }
    
    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member>? {
        fatalError("not implemented")
    }

}

extension OptionalAttribute: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension OptionalAttribute: Equatable {

    public static func == (_ lhs: OptionalAttribute, _ rhs: OptionalAttribute) -> Bool {
        return lhs.base == rhs.base
    }

}

extension OptionalAttribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(base)
    }

}
