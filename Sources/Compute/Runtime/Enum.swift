import ComputeCxx

struct AGTypeApplyEnumDataContext {
    let body: (Int, Any.Type, UnsafeRawPointer) -> Void
}

public func withUnsafePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeRawPointer) -> Void
) -> Bool {
    return withoutActuallyEscaping(body) { escapingBody in
        let context = AGTypeApplyEnumDataContext(body: escapingBody)
        return withUnsafePointer(to: context) { contextPointer in
            return __AGTypeApplyEnumData(
                Metadata(Value.self),
                enumValue,
                {
                    $0.assumingMemoryBound(to: AGTypeApplyEnumDataContext.self).pointee.body(Int($1), $2.type, $3)
                },
                contextPointer
            )
        }
    }
}

struct AGTypeApplyMutableEnumDataContext {
    let body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
}

public func withUnsafeMutablePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>,
    do body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
) -> Bool {
    return withoutActuallyEscaping(body) { escapingBody in
        let context = AGTypeApplyMutableEnumDataContext(body: escapingBody)
        return withUnsafePointer(to: context) { contextPointer in
            return __AGTypeApplyMutableEnumData(
                Metadata(Value.self),
                enumValue,
                {
                    $0.assumingMemoryBound(to: AGTypeApplyMutableEnumDataContext.self).pointee.body(Int($1), $2.type, $3)
                },
                contextPointer
            )
        }
    }
}
