import ComputeCxx

public struct AnyOptionalAttribute {

    public static var current: AnyOptionalAttribute {
        return AnyOptionalAttribute(AnyAttribute.current)
    }

    public var identifier: AnyAttribute

    public init() {
        identifier = .nil
    }

    public init(_ weakAttribute: AnyWeakAttribute) {
        identifier = __AGWeakAttributeGetAttribute(weakAttribute)
    }

    public init(_ attribute: AnyAttribute?) {
        identifier = attribute ?? .nil
    }

    public init(_ attribute: AnyAttribute) {
        identifier = attribute
    }

    public init<Value>(_ optionalAttribute: OptionalAttribute<Value>) {
        self = optionalAttribute.base
    }

    public func unsafeCast<Value>(to _: Value.Type) -> OptionalAttribute<Value> {
        return OptionalAttribute<Value>(base: self)
    }

    public var attribute: AnyAttribute? {
        get {
            return identifier == .nil ? nil : identifier
        }
        set {
            identifier = newValue ?? .nil
        }
    }

    public func map<T>(_ transform: (AnyAttribute) -> T) -> T? {
        if let attribute = attribute {
            return transform(attribute)
        } else {
            return nil
        }
    }

}

extension AnyOptionalAttribute: CustomStringConvertible {

    public var description: String {
        return attribute?.description ?? "nil"
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
