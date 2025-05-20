import ComputeCxx

extension Metadata {

    @_extern(c, "AGTypeApplyEnumData")
    static func applyEnumData(
        type: Metadata,
        value: UnsafeRawPointer,
        body: (Int, Metadata, UnsafeRawPointer) -> Void
    ) -> Bool

}

public func withUnsafePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeRawPointer) -> Void
) -> Bool {
    return Metadata.applyEnumData(type: Metadata(Value.self), value: enumValue) { tag, fieldType, fieldValue in
        body(tag, fieldType.type, fieldValue)
    }
}

@_extern(c, "AGTypeApplyMutableEnumData")
func applyMutableEnumData(
    type: Metadata,
    value: UnsafeRawPointer,
    body: (Int, Metadata, UnsafeMutableRawPointer) -> Void
) -> Bool

public func withUnsafeMutablePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
) -> Bool {
    return applyMutableEnumData(type: Metadata(Value.self), value: enumValue) { tag, fieldType, fieldValue in
        body(tag, fieldType.type, fieldValue)
    }
}
