@propertyWrapper
@dynamicMemberLookup
public struct WeakAttribute<Value> {

    public var base: AnyWeakAttribute

    public init(base: AnyWeakAttribute) {
        self.base = base
    }
    
    public init() {
        fatalError("not implemented")
    }
    
    public init(_ attribute: Attribute<Value>) {
        fatalError("not implemented")
    }
    
    public init(_ attribute: Attribute<Value>?) {
        fatalError("not implemented")
    }

    public func changedValue(options: ValueOptions) -> (value: Value, changed: Bool)? {
        fatalError("not implemented")
    }

    public var value: Value? {
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

extension WeakAttribute: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension WeakAttribute: Equatable {

    public static func == (_ lhs: WeakAttribute, _ rhs: WeakAttribute) -> Bool {
        return lhs.base == rhs.base
    }

}

extension WeakAttribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(base)
    }

}
