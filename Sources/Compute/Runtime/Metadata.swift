import ComputeCxx

extension Metadata {

    @_extern(c, "AGTypeApplyFields")
    static func applyFields(type: Metadata, body: (UnsafePointer<CChar>, Int, Metadata) -> Void)

}

public func forEachField(of type: Any.Type, do body: (UnsafePointer<Int8>, Int, Any.Type) -> Void) {
    Metadata.applyFields(type: Metadata(type)) { fieldName, fieldOffset, fieldType in
        body(fieldName, fieldOffset, fieldType.type)
    }
}

extension Metadata {

    public init(_ type: Any.Type) {
        self.init(rawValue: unsafeBitCast(type, to: UnsafePointer<_Metadata>.self))
    }

    public var type: Any.Type {
        return unsafeBitCast(rawValue, to: Any.Type.self)
    }

    @_extern(c, "AGTypeApplyFields2")
    static func applyFields2(
        type: Metadata,
        options: Metadata.ApplyOptions,
        body: (UnsafePointer<CChar>, Int, Metadata) -> Bool
    )
        -> Bool

    public func forEachField(
        options: Metadata.ApplyOptions,
        do body: (UnsafePointer<CChar>, Int, Any.Type) -> Bool
    )
        -> Bool
    {
        return Metadata.applyFields2(type: self, options: options) { fieldName, fieldOffset, fieldType in
            return body(fieldName, fieldOffset, fieldType.type)
        }
    }

}

extension Metadata: @retroactive CustomStringConvertible {

    public var description: String {
        #if os(macOS)
        return __AGTypeDescription(self) as String
        #else
        return String(__AGTypeCopyDescription(self))
        #endif
    }

}

extension Metadata: @retroactive Equatable {}

extension Metadata: @retroactive Hashable {}
