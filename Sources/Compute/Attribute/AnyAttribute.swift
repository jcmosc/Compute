import ComputeCxx

public struct InputOptions {}

public struct SearchOptions {}

public struct AttributeFlags {}

extension AnyAttribute {

    public static var current: AnyAttribute? {
        fatalError("not implemented")
    }

    public init<Value>(_ attribute: Attribute<Value>) {
        fatalError("not implemented")
    }

    public func unsafeOffset(at offset: Int) -> AnyAttribute {
        fatalError("not implemented")
    }

    public func unsafeCast<Value>(to type: Value.Type) -> Attribute<Value> {
        fatalError("not implemented")
    }

    public var _bodyType: Any.Type {
        fatalError("not implemented")
    }

    public var _bodyPointer: UnsafeRawPointer {
        fatalError("not implemented")
    }

    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        fatalError("not implemented")
    }

    public func mutateBody<Body>(as type: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        fatalError("not implemented")
    }

    public func breadthFirstSearch(options: SearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        fatalError("not implemented")
    }

    public var valueType: Any.Type {
        fatalError("not implemented")
    }

    public func setFlags(_ newFlags: AttributeFlags, mask: AttributeFlags) {
        fatalError("not implemented")
    }

    public func addInput(_ input: AnyAttribute, options: InputOptions, token: Int) {
        fatalError("not implemented")
    }

    public func addInput<T>(_ input: Attribute<T>, options: InputOptions, token: Int) {
        fatalError("not implemented")
    }

    public var indirectDependency: AnyAttribute? {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

}

extension AnyAttribute: @retroactive CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension AnyAttribute: @retroactive Equatable {

    public static func == (_ lhs: AnyAttribute, _ rhs: AnyAttribute) -> Bool {
        fatalError("not implemented")
    }

}

extension AnyAttribute: @retroactive Hashable {

    public func hash(into hasher: inout Hasher) {
        fatalError("not implemented")
    }

}
