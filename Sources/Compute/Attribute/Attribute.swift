import ComputeCxx

public struct ValueState {}

public struct ValueOptions {}

public struct ChangedValueFlags {}

@propertyWrapper
@dynamicMemberLookup
public struct Attribute<Value> {

    public var identifier: AnyAttribute

    public init(identifier: AnyAttribute) {
        self.identifier = identifier
    }

    public init(_ attribute: Attribute<Value>) {
        fatalError("not implemented")
    }

    public init(type: Value.Type) {
        fatalError("not implemented")
    }

    public init(value: Value) {
        fatalError("not implemented")
    }

    public init<Body: _AttributeBody>(
        body: UnsafePointer<Body>, value: UnsafePointer<Value>?, flags: AttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) {
        fatalError("not implemented")
    }

    public func unsafeCast<T>(to type: T.Type) -> Attribute<T> {
        fatalError("not implemented")
    }

    public func unsafeOffset<T>(at offset: Int, as type: T) -> Attribute<T> {
        fatalError("not implemented")
    }

    public func applying<Member>(offset: PointerOffset<Value, Member>) -> Attribute<Member> {
        fatalError("not implemented")
    }

    public func breadthFirstSearch(options: SearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        fatalError("not implemented")
    }

    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        fatalError("not implemented")
    }

    public func mutateBody<Body>(as bodyType: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        fatalError("not implemented")
    }

    public func validate() {
        fatalError("not implemented")
    }

    public var value: Value {
        unsafeAddress {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public func setValue(_ value: Value) -> Bool {
        fatalError("not implemented")
    }

    public var hasValue: Bool {
        fatalError("not implemented")
    }

    public var valueState: ValueState {
        fatalError("not implemented")
    }

    public func prefetchValue() {
        fatalError("not implemented")
    }

    public func updateValue() {
        fatalError("not implemented")
    }

    public func invalidateValue() {
        fatalError("not implemented")
    }

    public func changedValue(options: ValueOptions) -> (value: Value, changed: Bool) {
        fatalError("not implemented")
    }

    public var flags: AttributeFlags {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public func setFlags(_ flags: AttributeFlags, mask: AttributeFlags) {
        fatalError("not implemented")
    }

    public func valueAndFlags(options: ValueOptions) -> (value: Value, flags: ChangedValueFlags) {
        fatalError("not implemented")
    }

    public func addInput<T>(_ input: Attribute<T>, options: InputOptions, token: Int) {
        fatalError("not implemented")
    }

    public func addInput(_ input: AnyAttribute, options: InputOptions, token: Int) {
        fatalError("not implemented")
    }

    public var graph: Graph {
        fatalError("not implemented")
    }

    public var subgraph: Subgraph {
        fatalError("not implemented")
    }

    public var subgraphOrNil: Subgraph? {
        fatalError("not implemented")
    }

    public var wrappedValue: Value {
        unsafeAddress {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public var projectedValue: Attribute<Value> {
        get {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public subscript<Member>(offset: (inout Value) -> PointerOffset<Value, Member>) -> Attribute<Member> {
        fatalError("not implemented")
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        fatalError("not implemented")
    }

}

extension Attribute: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension Attribute: Equatable {

    public static func == (_ lhs: Attribute, _ rhs: Attribute) -> Bool {
        return lhs.identifier == rhs.identifier
    }

}

extension Attribute: Hashable {

    public func hash(into hasher: inout Hasher) {
        hasher.combine(identifier)
    }

}

extension Attribute {

    public init<Body: Rule>(_ body: Body, initialValue: Value? = nil) where Body.Value == Value {
        fatalError("not implemented")
    }

    public init<Body: StatefulRule>(_ body: Body, initialValue: Value? = nil) where Body.Value == Value {
        fatalError("not implemented")
    }

}
