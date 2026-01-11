@propertyWrapper
@dynamicMemberLookup
public struct WeakAttribute<Value> {

    public var base: AnyWeakAttribute

    public init(base: AnyWeakAttribute) {
        self.base = base
    }

    public init() {
        base = AnyWeakAttribute(
            _details: AnyWeakAttribute.__Unnamed_struct__details(identifier: AnyAttribute(rawValue: 0), seed: 0)
        )
    }

    public init(_ attribute: Attribute<Value>) {
        base = AnyWeakAttribute(attribute.identifier)
    }

    public init(_ attribute: Attribute<Value>?) {
        base = AnyWeakAttribute(attribute?.identifier)
    }

    public func changedValue(options: AGValueOptions = []) -> (value: Value, changed: Bool)? {
        return attribute?.changedValue(options: options)
    }

    public var value: Value? {
        return __AGGraphGetWeakValue(base, [], Metadata(Value.self))
            .value?
            .assumingMemoryBound(to: Value.self)
            .pointee
    }

    public var attribute: Attribute<Value>? {
        get {
            return base.attribute?.unsafeCast(to: Value.self)
        }
        set {
            base.attribute = newValue?.identifier
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
        return attribute?[keyPath: keyPath]
    }

}

extension WeakAttribute: CustomStringConvertible {

    public var description: String {
        return base.description
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
