import ComputeCxx

struct AGGraphInternAttributeTypeThunk {
    let body: () -> UnsafeRawPointer
}

struct AGAttributeTypeUpdateThunk {
    let body: (UnsafeMutableRawPointer, AnyAttribute) -> Void
}

extension Graph {

    func internAttributeType(
        selfType: Any.Type,
        bodyType: _AttributeBody.Type,
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UInt32 {
        return withoutActuallyEscaping(update) { escapingUpdate in
            let thunk = AGGraphInternAttributeTypeThunk(body: {
                return withUnsafePointer(to: AGAttributeTypeUpdateThunk(body: escapingUpdate())) { updateThunkPointer in
                    let attributeType = AGAttributeType(
                        selfType: selfType,
                        bodyType: bodyType,
                        valueType: valueType,
                        flags: flags,
                        update: {
                            $0.assumingMemoryBound(to: AGAttributeTypeUpdateThunk.self).pointee.body($1, $2)
                        },
                        updateContext: updateThunkPointer
                    )
                    let pointer = UnsafeMutablePointer<AGAttributeType>.allocate(capacity: 1)
                    pointer.initialize(to: attributeType)
                    return UnsafeRawPointer(pointer)
                }
            })
            return withUnsafePointer(to: thunk) { thunkPointer in
                return __AGGraphInternAttributeType(
                    self,
                    Metadata(selfType),
                    {
                        return $0.assumingMemoryBound(to: AGGraphInternAttributeTypeThunk.self).pointee.body()
                    },
                    thunkPointer
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
        return withUnsafePointer(to: AGAttributeTypeUpdateThunk(body: update())) { updateThunkPointer in
            let attributeType = AGAttributeType(
                selfType: selfType,
                bodyType: bodyType,
                valueType: valueType,
                flags: flags,
                update: {
                    $0.assumingMemoryBound(to: AGAttributeTypeUpdateThunk.self).pointee.body($1, $2)
                },
                updateContext: updateThunkPointer
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
