import ComputeCxx
import Foundation

struct AGTypeApplyFieldsThunk {
    var body: (UnsafePointer<CChar>, Int, Any.Type) -> Void
}

struct AGTypeApplyFields2Thunk {
    var body: (UnsafePointer<CChar>, Int, Any.Type) -> Bool
}

public func forEachField(of type: Any.Type, do body: (UnsafePointer<Int8>, Int, Any.Type) -> Void) {

    withoutActuallyEscaping(body) { escapingClosure in
        withUnsafePointer(to: AGTypeApplyFieldsThunk(body: escapingClosure)) { thunkPointer in
            __AGTypeApplyFields(
                Metadata(type),
                {
                    $0.assumingMemoryBound(to: AGTypeApplyFieldsThunk.self).pointee.body($1, $2, $3.type)
                },
                thunkPointer
            )
        }
    }
}

extension Metadata {

    public init(_ type: Any.Type) {
        self.init(rawValue: unsafeBitCast(type, to: OpaquePointer.self))
    }

    public var type: Any.Type {
        return unsafeBitCast(rawValue, to: Any.Type.self)
    }

    public func forEachField(options: ApplyOptions, do body: (UnsafePointer<CChar>, Int, Any.Type) -> Bool)
        -> Bool
    {
        return withoutActuallyEscaping(body) { escapingClosure in
            return withUnsafePointer(to: AGTypeApplyFields2Thunk(body: escapingClosure)) { thunkPointer in
                return __AGTypeApplyFields2(
                    self,
                    options,
                    {
                        return $0.assumingMemoryBound(to: AGTypeApplyFields2Thunk.self).pointee.body($1, $2, $3.type)
                    },
                    thunkPointer
                )
            }
        }
    }

}

extension Metadata: @retroactive CustomStringConvertible {

    public var description: String {
        return __AGTypeDescription(self) as String
    }

}

extension Metadata: @retroactive Equatable {}

extension Metadata: @retroactive Hashable {}
