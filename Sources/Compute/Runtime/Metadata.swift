import ComputeCxx
import Foundation

public func forEachField(of type: Any.Type, do body: (UnsafePointer<Int8>, Int, Any.Type) -> Void) {
    struct Context {
        var body: (UnsafePointer<CChar>, Int, Any.Type) -> Void
    }

    withoutActuallyEscaping(body) { escapingClosure in
        var context = Context(body: escapingClosure)
        withUnsafeMutablePointer(to: &context) { contextPointer in
            __AGTypeApplyFields(
                Metadata(type),
                { name, offset, metadata, context in
                    guard let context = context?.assumingMemoryBound(to: Context.self).pointee else {
                        return
                    }
                    context.body(name, offset, metadata.type)
                }, contextPointer)
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
        struct Context {
            var body: (UnsafePointer<CChar>, Int, Any.Type) -> Bool
        }

        return withoutActuallyEscaping(body) { escapingClosure in
            var context = Context(body: escapingClosure)
            return withUnsafeMutablePointer(to: &context) { contextPointer in
                return __AGTypeApplyFields2(
                    self, options,
                    { name, offsetOrIndex, metadata, context in
                        guard let context = context?.assumingMemoryBound(to: Context.self).pointee else {
                            return false
                        }
                        return context.body(name, offsetOrIndex, metadata.type)
                    }, contextPointer)
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
