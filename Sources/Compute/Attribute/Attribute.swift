import ComputeCxx

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
            return withUnsafePointer(to: External<Value>()) { bodyPointer in
                return Attribute(body: bodyPointer, value: valuePointer, flags: .option16) {
                    return External<Value>._update
                }
            }
        }
    }

    public init(type: Value.Type) {
        self = withUnsafePointer(to: External<Value>()) { bodyPointer in
            // TODO: what are flags
            return Attribute(body: bodyPointer, value: nil, flags: .option16) {
                return External<Value>._update
            }
        }
    }

    public init<Body: _AttributeBody>(
        body: UnsafePointer<Body>,
        value: UnsafePointer<Value>?,
        flags: AGAttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) {

        guard let subgraph = Subgraph.current else {
            // or preconditionFailure...
            fatalError("attempting to create attribute with no subgraph: \(Body.self)")
        }

        let graph = subgraph.graph
        let typeID = graph.internAttributeType(
            selfType: Body.self,
            bodyType: Body.self,  // TODO: make this function generic, don't need to pass witness tables
            valueType: Value.self,
            flags: flags,
            update: update
        )
        identifier = __AGGraphCreateAttribute(typeID, body, value)
    }

    public func applying<Member>(offset: PointerOffset<Value, Member>) -> Attribute<Member> {
        return unsafeOffset(at: offset.byteOffset, as: Member.self)
    }

    public func breadthFirstSearch(options: AGSearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        return identifier.breadthFirstSearch(options: options, predicate)
    }

    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        identifier.visitBody(&visitor)
    }

    public func mutateBody<Body>(as bodyType: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        identifier.mutateBody(as: bodyType, invalidating: invalidating, mutator)
    }

    public func validate() {
        __AGGraphVerifyType(identifier, Metadata(Value.self))
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
        return __AGGraphHasValue(identifier)
    }

    public var valueState: AGValueState {
        return __AGGraphGetValueState(identifier)
    }

    public func prefetchValue() {
        __AGGraphPrefetchValue(identifier)
    }

    public func updateValue() {
        __AGGraphUpdateValue(identifier, 0)
    }

    public func invalidateValue() {
        __AGGraphInvalidateValue(identifier)
    }

    public func changedValue(options: AGValueOptions) -> (value: Value, changed: Bool) {
        let value = __AGGraphGetValue(identifier, options, Metadata(Value.self))
        return (
            value.value.assumingMemoryBound(to: Value.self).pointee,
            value.changed
        )
    }

    public var flags: AGAttributeFlags {
        get {
            return identifier.flags
        }
        set {
            identifier.flags = newValue
        }
    }

    public func setFlags(_ newFlags: AGAttributeFlags, mask: AGAttributeFlags) {
        identifier.setFlags(newFlags, mask: mask)
    }

    public func valueAndFlags(options: AGValueOptions) -> (value: Value, flags: AGChangedValueFlags) {
        let value = __AGGraphGetValue(identifier, options, Metadata(Value.self))
        return (
            value.value.assumingMemoryBound(to: Value.self).pointee,
            value.changed ? .changed : []
        )
    }

    public func addInput<T>(_ input: Attribute<T>, options: AGInputOptions, token: Int) {
        identifier.addInput(input, options: options, token: token)
    }

    public func addInput(_ input: AnyAttribute, options: AGInputOptions, token: Int) {
        identifier.addInput(input, options: options, token: token)
    }

    public var graph: Graph {
        // TODO: retain or not?
        return __AGGraphGetAttributeGraph(identifier).takeUnretainedValue()
    }

    public var subgraph: Subgraph {
        // TODO: retain or not?
        return __AGGraphGetAttributeSubgraph(identifier).takeUnretainedValue()
    }

    public var subgraphOrNil: Subgraph? {
        // TODO: retain or not?
        return __AGGraphGetAttributeSubgraph(identifier).takeUnretainedValue()
    }

    public var wrappedValue: Value {
        unsafeAddress {
            return __AGGraphGetValue(identifier, [], Metadata(Value.self))
                .value
                .assumingMemoryBound(to: Value.self)
        }
        nonmutating set {
            _ = setValue(newValue)
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
        unsafeOffset(at: PointerOffset.offset(body).byteOffset, as: Member.self)
    }

    public subscript<Member>(keyPath keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        if let offset = MemoryLayout<Value>.offset(of: keyPath) {
            return unsafeOffset(at: offset, as: Member.self)
        } else {
            return Attribute<Member>(Focus(root: self, keyPath: keyPath))
        }
    }

    public subscript<Member>(dynamicMember keyPath: KeyPath<Value, Member>) -> Attribute<Member> {
        self[keyPath: keyPath]
    }

    public func unsafeCast<T>(to type: T.Type) -> Attribute<T> {
        return identifier.unsafeCast(to: type)
    }

    public func unsafeOffset<Member>(at offset: Int, as _: Member.Type) -> Attribute<Member> {
        // TODO: fix Int/UInt32
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

    public init<Body: Rule>(_ body: Body) where Body.Value == Value {
        self = withUnsafePointer(to: body) { bodyPointer in
            return Attribute(body: bodyPointer, value: nil, flags: []) {
                return Body._update
            }
        }

    }

    public init<Body: Rule>(_ body: Body, initialValue: Body.Value) where Body.Value == Value {
        self = withUnsafePointer(to: body) { bodyPointer in
            return withUnsafePointer(to: initialValue) { initialValuePointer in
                return Attribute(body: bodyPointer, value: initialValuePointer, flags: []) {
                    return Body._update
                }
            }
        }

    }

    public init<Body: StatefulRule>(_ body: Body) where Body.Value == Value {
        self = withUnsafePointer(to: body) { bodyPointer in
            return Attribute(body: bodyPointer, value: nil, flags: []) {
                return Body._update
            }
        }
    }

    public init<Body: StatefulRule>(_ body: Body, initialValue: Body.Value) where Body.Value == Value {
        self = withUnsafePointer(to: body) { bodyPointer in
            return withUnsafePointer(to: initialValue) { initialValuePointer in
                return Attribute(body: bodyPointer, value: initialValuePointer, flags: []) {
                    return Body._update
                }
            }
        }
    }

}
