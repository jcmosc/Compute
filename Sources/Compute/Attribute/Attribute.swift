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
        self = attribute
    }

    public init(value: Value) {
        self = withUnsafePointer(to: value) { valuePointer in
            withUnsafePointer(to: External<Value>()) { bodyPointer in
                Attribute(body: bodyPointer, value: valuePointer, flags: .external) {
                    External<Value>._update
                }
            }
        }
    }

    public init(type: Value.Type) {
        self = withUnsafePointer(to: External<Value>()) { bodyPointer in
            Attribute(body: bodyPointer, value: nil, flags: .external) {
                External<Value>._update
            }
        }
    }

    public init<Body: _AttributeBody>(
        body: UnsafePointer<Body>,
        value: UnsafePointer<Value>?,
        flags: AGAttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) {
        guard let graphContext = Subgraph.currentGraphContext else {
            preconditionFailure("attempting to create attribute with no subgraph: \(Body.self)")
        }

        let typeID = graphContext.internAttributeType(
            type: Metadata(Body.self)
        ) {
            let attributeType = AGAttributeType(
                selfType: Body.self,
                valueType: Value.self,
                flags: flags,
                update: update()
            )
            let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
            pointer.initialize(to: attributeType)
            return UnsafePointer(pointer)
        }
        identifier = AnyAttribute(type: typeID, body: body, value: value)
    }

    public var graph: Graph {
        return identifier.graph
    }

    public var subgraph: Subgraph {
        return identifier.subgraph
    }

    public var subgraphOrNil: Subgraph? {
        return identifier.subgraphOrNil
    }
    
    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        identifier.visitBody(&visitor)
    }
    
    public var flags: AGAttributeFlags {
        get {
            return identifier.flags
        }
        nonmutating set {
            identifier.flags = newValue
        }
    }
    
    public func setFlags(_ newFlags: AGAttributeFlags, mask: AGAttributeFlags) {
        identifier.setFlags(newFlags, mask: mask)
    }

    public func applying<Member>(offset: PointerOffset<Value, Member>) -> Attribute<Member> {
        return unsafeOffset(at: offset.byteOffset, as: Member.self)
    }

    public func mutateBody<Body>(as bodyType: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        identifier.mutateBody(as: bodyType, invalidating: invalidating, mutator)
    }

    public func addInput<T>(_ input: Attribute<T>, options: AGInputOptions, token: Int) {
        identifier.addInput(input, options: options, token: token)
    }

    public func addInput(_ input: AnyAttribute, options: AGInputOptions, token: Int) {
        identifier.addInput(input, options: options, token: token)
    }
    
    public func breadthFirstSearch(options: AGSearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        return identifier.breadthFirstSearch(options: options, predicate)
    }

    public func validate() {
        identifier.verifyType(type: Metadata(Value.self))
    }

    public var value: Value {
        unsafeAddress {
            return __AGGraphGetValue(identifier, [], Metadata(Value.self))
                .value
                .assumingMemoryBound(to: Value.self)
        }
        nonmutating set {
            _ = setValue(newValue)
        }
    }

    public func setValue(_ value: Value) -> Bool {
        return withUnsafePointer(to: value) { valuePointer in
            return __AGGraphSetValue(identifier, valuePointer, Metadata(Value.self))
        }
    }

    public var hasValue: Bool {
        return identifier.hasValue
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

    public func valueAndFlags(options: ValueOptions) -> (value: Value, flags: ChangedValueFlags) {
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
            return self
        }
        set {
            self = newValue
        }
    }

    public subscript<Member>(offset body: (inout Value) -> PointerOffset<Value, Member>) -> Attribute<Member> {
        return unsafeOffset(at: PointerOffset.offset(body).byteOffset, as: Member.self)
    }

    public subscript<Member>(keyPath keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        if let offset = MemoryLayout<Value>.offset(of: keyPath) {
            return unsafeOffset(at: offset, as: Member.self)
        } else {
            return Attribute<Member>(Focus(root: self, keyPath: keyPath))
        }
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        return self[keyPath: keyPath]
    }

    public func unsafeCast<T>(to type: T.Type) -> Attribute<T> {
        return identifier.unsafeCast(to: type)
    }

    public func unsafeOffset<Member>(at offset: Int, as type: Member.Type) -> Attribute<Member> {
        return Attribute<Member>(
            identifier: __AGGraphCreateOffsetAttribute2(identifier, UInt32(offset), MemoryLayout<Member>.size)
        )
    }

}

extension Attribute: CustomStringConvertible {

    public var description: String {
        return identifier.description
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
