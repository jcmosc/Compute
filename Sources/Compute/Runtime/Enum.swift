import ComputeCxx

@_silgen_name("IAGTypeApplyEnumData")
func IAGTypeApplyEnumData(
    _ type: Metadata,
    value: UnsafeRawPointer,
    body: (Int, Metadata, UnsafeRawPointer) -> Void
) -> Bool

public func withUnsafePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeRawPointer) -> Void
) -> Bool {
    return IAGTypeApplyEnumData(Metadata(Value.self), value: enumValue) { tag, fieldType, fieldValue in
        body(tag, fieldType.type, fieldValue)
    }
}

@_silgen_name("IAGTypeApplyMutableEnumData")
func IAGTypeApplyMutableEnumData(
    _ type: Metadata,
    value: UnsafeRawPointer,
    body: (Int, Metadata, UnsafeMutableRawPointer) -> Void
) -> Bool

public func withUnsafeMutablePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
) -> Bool {
    return IAGTypeApplyMutableEnumData(Metadata(Value.self), value: enumValue) { tag, fieldType, fieldValue in
        body(tag, fieldType.type, fieldValue)
    }
}
