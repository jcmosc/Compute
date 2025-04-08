import ComputeCxx

extension Graph {

    struct InternAttributeTypeContext {
        let selfType: Any.Type
        let bodyType: _AttributeBody.Type
        let valueType: Any.Type
        let flags: AGAttributeTypeFlags
        let update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    }

    func internAttributeType(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UInt32 {
        return withoutActuallyEscaping(update) { escapingUpdate in
            let context = InternAttributeTypeContext(
                selfType: selfType,
                bodyType: bodyType,
                valueType: valueType,
                flags: flags,
                update: escapingUpdate
            )
            return withUnsafePointer(to: context) { contextPointer in
                return __AGGraphInternAttributeType(
                    self,
                    Metadata(selfType),
                    { contextPointer in
                        // FUN_1afe82038
                        let context = contextPointer.assumingMemoryBound(to: InternAttributeTypeContext.self).pointee
                        let pointer = AGAttributeType.allocate(
                            selfType: context.selfType,
                            bodyType: context.bodyType,
                            valueType: context.valueType,
                            flags: context.flags,
                            update: context.update
                        )
                        return UnsafeRawPointer(pointer)
                    },
                    contextPointer
                )
            }
        }
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
        struct Context {
            let update: (UnsafeMutableRawPointer, AnyAttribute) -> Void
        }
        return withUnsafePointer(to: Context(update: update())) { contextPointer in
            let attributeType = AGAttributeType(
                selfType: selfType,
                bodyType: bodyType,
                valueType: valueType,
                flags: flags,
                update: { context, body, attribute in
                    context.assumingMemoryBound(to: Context.self).pointee.update(body, attribute)
                },
                updateContext: contextPointer
            )
            let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
            pointer.initialize(to: attributeType)
            return pointer
        }
    }

    // sub_1AFE86960
    init(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,  // witness table
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: @convention(c) (UnsafeRawPointer, UnsafeMutableRawPointer, AnyAttribute) -> Void,
        updateContext: UnsafeRawPointer?
    ) {
        self.init()

        var effectiveFlags = flags
        effectiveFlags.insert(bodyType.flags)
        effectiveFlags.insert(AGAttributeTypeFlags(rawValue: UInt32(bodyType.comparisonMode.rawValue)))
        if bodyType._hasDestroySelf && !effectiveFlags.contains(.option4) {
            effectiveFlags.insert(.option4)  // TODO: is this flag reallyHasDestroySelf??
        }

        self.body_type_id = Metadata(selfType)
        self.value_type_id = Metadata(valueType)
        self.update_function = update
        self.update_function_context = updateContext
        self.callbacks = nil
        self.flags = effectiveFlags

        self.initial_body_type_id = Metadata(selfType)
        self.initial_body_witness_table = Metadata(bodyType)  // not sure if Metadata works for witness tables
    }
}
