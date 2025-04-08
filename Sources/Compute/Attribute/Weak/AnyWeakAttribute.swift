import ComputeCxx

extension AnyWeakAttribute {

    public init<Value>(_ attribute: WeakAttribute<Value>) {
        self = attribute.base
    }

    public init(_ attribute: AnyAttribute?) {
        self = __AGCreateWeakAttribute(attribute ?? .nil)
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

    public func unsafeCast<Value>(to type: Value.Type) -> WeakAttribute<Value> {
        return WeakAttribute<Value>(base: self)
    }

}

extension AnyWeakAttribute: @retroactive CustomStringConvertible {

    public var description: String {
        return attribute?.description ?? "nil"
    }

}

extension AnyWeakAttribute: @retroactive Equatable {}

extension AnyWeakAttribute: @retroactive Hashable {}
