import ComputeCxx

extension Graph {

    @_extern(c, "AGGraphInternAttributeType")
    fileprivate static func internAttributeType(
        _ graph: UnsafeRawPointer,
        type: Metadata,
        makeAttributeType: () -> UnsafeRawPointer
    )
        -> UInt32

}

extension AGUnownedGraphRef {

    @inline(__always)
    func internAttributeType(type: Metadata, makeAttributeType: () -> UnsafeRawPointer) -> UInt32 {
        return Graph.internAttributeType(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            type: type,
            makeAttributeType: makeAttributeType
        )
    }

}

extension AGAttributeType {

    // FUN_1afe864c4
    static func allocate(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UnsafeMutablePointer<AGAttributeType> {

        let attributeType = AGAttributeType(
            selfType: selfType,
            bodyType: bodyType,
            valueType: valueType,
            flags: flags,
            update: update()
        )
        let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
        pointer.initialize(to: attributeType)
        return pointer

    }

    // sub_1AFE86960
    init(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,  // witness table
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: @escaping (UnsafeMutableRawPointer, AnyAttribute) -> Void,
    ) {
        self.init()

        var effectiveFlags = flags
        effectiveFlags.insert(bodyType.flags)
        effectiveFlags.insert(AGAttributeTypeFlags(rawValue: UInt32(bodyType.comparisonMode.rawValue)))
        if bodyType._hasDestroySelf && !effectiveFlags.contains(.option4) {
            effectiveFlags.insert(.option4)  // TODO: is this flag reallyHasDestroySelf??
        }

        self.typeID = Metadata(selfType)
        self.valueTypeID = Metadata(valueType)
        self.update_function = unsafeBitCast(update, to: AGClosureStorage2.self)
        self.callbacks = nil
        self.flags = effectiveFlags

        self.initial_body_type_id = Metadata(selfType)
        self.initial_body_witness_table = Metadata(bodyType)  // not sure if Metadata works for witness tables
    }
}
