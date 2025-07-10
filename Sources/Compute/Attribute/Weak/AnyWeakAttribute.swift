import ComputeCxx

extension AnyWeakAttribute {

    public init<Value>(_ attribute: WeakAttribute<Value>) {
        self = attribute.base
    }

    public init(_ attribute: AnyAttribute?) {
        self = __AGCreateWeakAttribute(attribute ?? .nil)
    }

    public func unsafeCast<Value>(to type: Value.Type) -> WeakAttribute<Value> {
        return WeakAttribute<Value>(base: self)
    }

    public var attribute: AnyAttribute? {
        get {
            let attribute = __AGWeakAttributeGetAttribute(self)
            return attribute == .nil ? nil : attribute
        }
        set {
            self = AnyWeakAttribute(newValue)
        }
    }

}

extension AnyWeakAttribute: @retroactive CustomStringConvertible {

    public var description: String {
        return attribute?.description ?? "nil"
    }

}

extension AnyWeakAttribute: @retroactive Equatable {

    public static func == (lhs: AnyWeakAttribute, rhs: AnyWeakAttribute) -> Bool {
        return lhs._details.identifier == rhs._details.identifier
            && lhs._details.seed == rhs._details.seed
    }

}

extension AnyWeakAttribute: @retroactive Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(_details.identifier)
        hasher.combine(_details.seed)
    }

}
