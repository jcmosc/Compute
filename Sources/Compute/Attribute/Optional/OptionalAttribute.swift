@propertyWrapper
@dynamicMemberLookup
public struct OptionalAttribute<Value> {

    public var base: AnyOptionalAttribute

    public init(base: AnyOptionalAttribute) {
        self.base = base
    }

    public init() {
        base = AnyOptionalAttribute()
    }

    public init(_ weakAttribute: WeakAttribute<Value>) {
        base = AnyOptionalAttribute(weakAttribute.base)
    }

    public init(_ attribute: Attribute<Value>) {
        base = AnyOptionalAttribute(attribute.identifier)
    }

    public init(_ attribute: Attribute<Value>?) {
        base = AnyOptionalAttribute(attribute?.identifier)
    }

    public var attribute: Attribute<Value>? {
        get {
            return base.attribute?.unsafeCast(to: Value.self)
        }
        set {
            base.attribute = newValue?.identifier
        }
    }

    public var value: Value? {
        return attribute?.value
    }

    public func changedValue(options: AGValueOptions = []) -> (value: Value, changed: Bool)? {
        return attribute?.changedValue(options: options)
    }

    public func map<T>(_ transform: (Attribute<Value>) -> T) -> T? {
        if let attribute = attribute {
            return transform(attribute)
        } else {
            return nil
        }
    }

    public var wrappedValue: Value? {
        return value
    }

    public var projectedValue: Attribute<Value>? {
        get {
            return attribute
        }
        set {
            attribute = newValue
        }
        _modify {
            yield &attribute
        }
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member>? {
        return attribute?[dynamicMember: keyPath]
    }

}

extension OptionalAttribute: CustomStringConvertible {

    public var description: String {
        return attribute?.description ?? "nil"
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
