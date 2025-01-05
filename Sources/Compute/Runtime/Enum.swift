public func withUnsafePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>, do body: (Int, Any.Type, UnsafeRawPointer) -> Void
) -> Bool {
    fatalError("not implemented")
}

public func withUnsafeMutablePointerToEnumCase<Value>(
    of enumValue: UnsafeMutablePointer<Value>, do body: (Int, Any.Type, UnsafeMutableRawPointer) -> Void
) -> Bool {
    fatalError("not implemented")
}
