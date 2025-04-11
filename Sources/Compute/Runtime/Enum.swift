import ComputeCxx

struct AGTypeApplyEnumDataThunk {
    let body: (Int, Any.Type, UnsafeRawPointer) -> Void
}

public func withUnsafePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeRawPointer) -> Void
) -> Bool {
    return withoutActuallyEscaping(body) { escapingBody in
        return withUnsafePointer(to: AGTypeApplyEnumDataThunk(body: escapingBody)) { thunkPointer in
            return __AGTypeApplyEnumData(
                Metadata(Value.self),
                enumValue,
                {
                    $0.assumingMemoryBound(to: AGTypeApplyEnumDataThunk.self).pointee.body(Int($1), $2.type, $3)
                },
                thunkPointer
            )
        }
    }
}

struct AGTypeApplyMutableEnumDatThunk {
    let body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
}

public func withUnsafeMutablePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
) -> Bool {
    return withoutActuallyEscaping(body) { escapingBody in
        return withUnsafePointer(to: AGTypeApplyMutableEnumDatThunk(body: escapingBody)) { thunkPointer in
            return __AGTypeApplyMutableEnumData(
                Metadata(Value.self),
                enumValue,
                {
                    $0.assumingMemoryBound(to: AGTypeApplyMutableEnumDatThunk.self).pointee.body(Int($1), $2.type, $3)
                },
                thunkPointer
            )
        }
    }
}
