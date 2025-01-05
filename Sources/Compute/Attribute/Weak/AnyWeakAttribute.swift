public struct AnyWeakAttribute {

    public init<Value>(_ attribute: WeakAttribute<Value>) {
        fatalError("not implemented")
    }

    public init(_ attribute: AnyAttribute?) {
        fatalError("not implemented")
    }

    public func unsafeCast<Value>(to type: Value.Type) -> WeakAttribute<Value> {
        fatalError("not implemented")
    }

    public var attribute: AnyAttribute? {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

}

extension AnyWeakAttribute: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension AnyWeakAttribute: Equatable {

    public static func == (lhs: AnyWeakAttribute, rhs: AnyWeakAttribute) -> Bool {
        fatalError("not implemented")
    }

}

extension AnyWeakAttribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        fatalError("not implemented")
    }

}
