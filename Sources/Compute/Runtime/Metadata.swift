import ComputeCxx
import Foundation

extension Metadata {

    public init(_ type: Any.Type) {
        self.init(rawValue: unsafeBitCast(type, to: OpaquePointer.self))
    }

    public var type: Any.Type {
        return unsafeBitCast(rawValue, to: Any.Type.self)
    }

    public struct ApplyOptions {

    }

    public func forEachField(options: ApplyOptions, do body: @escaping (UnsafePointer<Int8>, Int, Any.Type) -> Bool)
        -> Bool
    {
        fatalError("not implemented")
    }

}

extension Metadata: @retroactive CustomStringConvertible {

    public var description: String {
        return __AGTypeDescription(self) as String
    }

}

extension Metadata: @retroactive Equatable {}

extension Metadata: @retroactive Hashable {}
