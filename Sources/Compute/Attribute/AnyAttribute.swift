import ComputeCxx

struct AGGraphMutateAttributeThunk {
    let body: (UnsafeMutableRawPointer) -> Void
}

struct AGGraphSearchThunk {
    let body: (AnyAttribute) -> Bool
}

extension AnyAttribute {

    public static var current: AnyAttribute? {
        let attribute = __AGGraphGetCurrentAttribute()
        return attribute == .nil ? nil : attribute
    }

    public init<Value>(_ attribute: Attribute<Value>) {
        self = attribute.identifier
    }

    public func unsafeOffset(at offset: Int) -> AnyAttribute {
        // TODO: swift Int instead of UInt32
        return __AGGraphCreateOffsetAttribute(self, UInt32(offset))
    }

    public func unsafeCast<Value>(to type: Value.Type) -> Attribute<Value> {
        return Attribute<Value>(identifier: self)
    }

    public func visitBody<Visitor: AttributeBodyVisitor>(_ visitor: inout Visitor) {
        let info = __AGGraphGetAttributeInfo(self)
        let type = info.type.pointee.body_type_id.type as! _AttributeBody.Type
        type._visitSelf(info.body, visitor: &visitor)
    }

    // XXX: Swift compiler crashes when capturing a generic type
    public func mutateBody<Body>(as type: Body.Type, invalidating: Bool, _ mutator: (inout Body) -> Void) {
        withoutActuallyEscaping(mutator) { escapingMutator in
            let thunk = AGGraphMutateAttributeThunk(body: { bodyPointer in
                escapingMutator(&bodyPointer.assumingMemoryBound(to: Body.self).pointee)
            })
            withUnsafePointer(to: thunk) { thunkPointer in
                __AGGraphMutateAttribute(
                    self,
                    Metadata(type),
                    invalidating,
                    {
                        $0.assumingMemoryBound(to: AGGraphMutateAttributeThunk.self).pointee.body($1)
                    },
                    thunkPointer
                )
            }
        }

    }

    public func breadthFirstSearch(options: AGSearchOptions, _ predicate: (AnyAttribute) -> Bool) -> Bool {
        // TODO: @silgen?
        return withoutActuallyEscaping(predicate) { escapingPredicate in
            return withUnsafePointer(to: AGGraphSearchThunk(body: escapingPredicate)) { thunkPointer in
                return __AGGraphSearch(
                    self,
                    options,
                    {
                        $0.assumingMemoryBound(to: AGGraphSearchThunk.self).pointee.body($1)
                    },
                    thunkPointer
                )
            }
        }
    }

    public var flags: AGAttributeFlags {
        get {
            return __AGGraphGetFlags(self)
        }
        set {
            __AGGraphSetFlags(self, newValue)
        }
    }

    public func setFlags(_ newFlags: AGAttributeFlags, mask: AGAttributeFlags) {
        let oldFlags = __AGGraphGetFlags(self)
        let newFlags = oldFlags.subtracting(mask).union(newFlags.intersection(mask))
        __AGGraphSetFlags(self, newFlags)
    }

    public func addInput(_ input: AnyAttribute, options: AGInputOptions, token: Int) {
        __AGGraphAddInput(self, input, options)
    }

    public func addInput<T>(_ input: Attribute<T>, options: AGInputOptions, token: Int) {
        addInput(input.identifier, options: options, token: token)
    }

    public var indirectDependency: AnyAttribute? {
        get {
            let indirectDependency = __AGGraphGetIndirectDependency(self)
            return indirectDependency == .nil ? nil : indirectDependency
        }
        set {
            __AGGraphSetIndirectDependency(self, newValue ?? .nil)
        }
    }

    public var _bodyType: Any.Type {
        let info = __AGGraphGetAttributeInfo(self)
        return info.type.pointee.body_type_id.type
    }

    public var _bodyPointer: UnsafeRawPointer {
        let info = __AGGraphGetAttributeInfo(self)
        return info.body
    }

    public var valueType: Any.Type {
        let info = __AGGraphGetAttributeInfo(self)
        return info.type.pointee.value_type_id.type
    }

}

extension AnyAttribute: @retroactive CustomStringConvertible {

    public var description: String {
        return "#\(rawValue)"
    }

}

extension AnyAttribute: @retroactive Equatable {}

extension AnyAttribute: @retroactive Hashable {}
