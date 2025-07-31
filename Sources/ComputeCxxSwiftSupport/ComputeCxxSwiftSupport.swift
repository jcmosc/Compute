import Foundation

@_silgen_name("AGDispatchEquatable")
public func Equatable_isEqual_indirect<T: Equatable>(
    _ lhs: UnsafePointer<T>,
    _ rhs: UnsafePointer<T>
) -> Bool {
    return lhs.pointee == rhs.pointee
}

@_silgen_name("AGSetTypeForKey")
public func setTypeForKey(
    _ dict: NSMutableDictionary,
    _ key: NSString,
    _ type: Any.Type
) {
    dict.setObject(type, forKey: key)
}
