import ComputeCxx

extension Graph {

    @_extern(c, "AGGraphSearch")
    static func search(attribute: AnyAttribute, options: SearchOptions, predicate: @escaping (AnyAttribute) -> Bool) -> Bool

    @_extern(c, "AGGraphMutateAttribute")
    static func mutateAttribute(
        _ attribute: AnyAttribute,
        type: Metadata,
        invalidating: Bool,
        modify: (UnsafeMutableRawPointer) -> Void
    )

}

extension AnyAttribute {

    public static var current: AnyAttribute? {
        let attribute = __AGGraphGetCurrentAttribute()
        return attribute == .nil ? nil : attribute
    }

    public init<Value>(_ attribute: Attribute<Value>) {
        self = attribute.identifier
    }

    public func unsafeCast<Value>(to type: Value.Type) -> Attribute<Value> {
        return Attribute<Value>(identifier: self)
    }

    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        info.type.pointee.attributeBody._visitSelf(info.body, visitor: &visitor)
    }

    public func mutateBody<Body>(as type: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        Graph.mutateAttribute(self, type: Metadata(type), invalidating: invalidating) { pointer in
            mutator(&pointer.assumingMemoryBound(to: Body.self).pointee)
        }
    }

    // Node

    public func setFlags(_ newFlags: Subgraph.Flags, mask: Subgraph.Flags) {
        flags = flags.subtracting(mask).union(newFlags.intersection(mask))
    }

    public func addInput(_ input: AnyAttribute, options: AGInputOptions, token: Int) {
        addInput(input, options: options)
    }

    public func addInput<T>(_ input: Attribute<T>, options: AGInputOptions, token: Int) {
        addInput(input.identifier, options: options, token: token)
    }

    // Indirect Node

    public func unsafeOffset(at offset: Int) -> AnyAttribute {
        return __AGGraphCreateOffsetAttribute(self, UInt32(offset))
    }

    public var indirectDependency: AnyAttribute? {
        get {
            let indirectDependency = __AGGraphGetIndirectDependency(self)
            return indirectDependency == .nil ? nil : indirectDependency
        }
        nonmutating set {
            __AGGraphSetIndirectDependency(self, newValue ?? .nil)
        }
    }

    public func breadthFirstSearch(options: SearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        return withoutActuallyEscaping(predicate) { escapingPredicate in
            return Graph.search(attribute: self, options: options, predicate: escapingPredicate)
        }
    }

    public var _bodyType: Any.Type {
        return info.type.pointee.self_id.type
    }

    public var _bodyPointer: UnsafeRawPointer {
        return info.body
    }

    public var valueType: Any.Type {
        return info.type.pointee.value_id.type
    }

}

extension AnyAttribute: @retroactive CustomStringConvertible {

    public var description: String {
        return "#\(rawValue)"
    }

}

extension AnyAttribute: @retroactive Equatable {}

extension AnyAttribute: @retroactive Hashable {}
