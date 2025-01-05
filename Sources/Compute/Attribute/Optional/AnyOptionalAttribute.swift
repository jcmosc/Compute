public struct AnyOptionalAttribute {

    public static var current: AnyOptionalAttribute? {
        fatalError("not implemented")
    }

    public var identifier: AnyAttribute

    public init() {
        fatalError("not implemented")
    }

    public init(_ weakAttribute: AnyWeakAttribute) {
        fatalError("not implemented")
    }

    public init(_ attribute: AnyAttribute?) {
        fatalError("not implemented")
    }

    public init(_ attribute: AnyAttribute) {
        fatalError("not implemented")
    }

    public init<Value>(_ optionalAttribute: OptionalAttribute<Value>) {
        fatalError("not implemented")
    }

    public func unsafeCast<Value>(to _: Value.Type) -> OptionalAttribute<Value> {
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

    public func map<T>(_ transform: (AnyAttribute) -> T) -> T? {
        fatalError("not implemented")
    }

}

extension AnyOptionalAttribute: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension AnyOptionalAttribute: Equatable {

    public static func == (_ lhs: AnyOptionalAttribute, _ rhs: AnyOptionalAttribute) -> Bool {
        return lhs.identifier == rhs.identifier
    }

}

extension AnyOptionalAttribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(identifier)
    }

}
