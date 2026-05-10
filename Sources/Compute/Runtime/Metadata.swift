import ComputeCxx

@_silgen_name("AGTypeApplyFields")
func AGTypeApplyFields(_ type: Metadata, body: (UnsafePointer<CChar>, Int, Metadata) -> Void)

public func forEachField(of type: Any.Type, do body: (UnsafePointer<Int8>, Int, Any.Type) -> Void) {
    AGTypeApplyFields(Metadata(type)) { fieldName, fieldOffset, fieldType in
        body(fieldName, fieldOffset, fieldType.type)
    }
}

@_silgen_name("AGTypeApplyFields2")
func AGTypeApplyFields2(
    _ type: Metadata,
    options: Metadata.ApplyOptions,
    body: (UnsafePointer<CChar>, Int, Metadata) -> Bool
) -> Bool

extension Metadata {

    public init(_ type: Any.Type) {
        self.init(rawValue: unsafeBitCast(type, to: UnsafePointer<_Metadata>.self))
    }

    public var type: Any.Type {
        return unsafeBitCast(rawValue, to: Any.Type.self)
    }

    public func forEachField(
        options: Metadata.ApplyOptions,
        do body: (UnsafePointer<CChar>, Int, Any.Type) -> Bool
    )
        -> Bool
    {
        return AGTypeApplyFields2(self, options: options) { fieldName, fieldOffset, fieldType in
            return body(fieldName, fieldOffset, fieldType.type)
        }
    }

}

extension Metadata: @retroactive CustomStringConvertible {

    public var description: String {
        #if os(iOS) || os(macOS)
        return __AGTypeDescription(self) as String
        #else
        return String(__AGTypeCopyDescription(self))
        #endif
    }

}

extension Metadata: @retroactive Equatable {}

extension Metadata: @retroactive Hashable {}
