// Called from ComputeCxx
@_silgen_name("AGDispatchEquatable")
public func Equatable_isEqual_indirect<T: Equatable>(
    _ lhs: UnsafePointer<T>,
    _ rhs: UnsafePointer<T>
) -> Bool {
    return lhs.pointee == rhs.pointee
}
