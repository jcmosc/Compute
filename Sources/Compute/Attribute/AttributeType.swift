import ComputeCxx

extension AGUnownedGraphRef {

    @_extern(c, "AGGraphInternAttributeType")
    fileprivate static func internAttributeType(
        _ graph: UnsafeRawPointer,
        type: Metadata,
        makeAttributeType: () -> UnsafePointer<AGAttributeType>
    )
        -> UInt32

    @inline(__always)
    func internAttributeType(type: Metadata, makeAttributeType: () -> UnsafePointer<AGAttributeType>) -> UInt32 {
        return AGUnownedGraphRef.internAttributeType(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            type: type,
            makeAttributeType: makeAttributeType
        )
    }

}

extension AGAttributeType {

    init(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,  // witness table
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: @escaping (UnsafeMutableRawPointer, AnyAttribute) -> Void,
    ) {
        self.init()

        var flags = flags
        flags.insert(bodyType.flags)
        flags.insert(AGAttributeTypeFlags(rawValue: UInt32(bodyType.comparisonMode.rawValue)))
        if bodyType._hasDestroySelf {
            flags.insert(.hasDestroySelf)
        }

        self.selfType = Metadata(selfType)
        self.valueType = Metadata(valueType)
        self.update = unsafeBitCast(update, to: AGClosureStorage.self)
        self.callbacks = nil
        self.flags = flags

        self.initialSelfType = Metadata(selfType)
        self.initialAttributeBodyWitnessTable = unsafeBitCast(bodyType, to: UnsafeRawPointer.self)  // TODO: double check this
    }
}
