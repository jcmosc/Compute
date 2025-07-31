import ComputeCxx
import Foundation

struct ProtocolConformance {
    var metadata: Metadata
    var witnessTable: UnsafeRawPointer
}

extension AGUnownedGraphRef {

    @_extern(c, "AGGraphInternAttributeType")
    fileprivate static func internAttributeType(
        _ graph: UnsafeRawPointer,
        type: Metadata,
        makeAttributeType: () -> UnsafePointer<AGAttributeType>
    )
        -> UInt32

    @inline(__always)
    func internAttributeType(
        type: Metadata,
        makeAttributeType: () -> UnsafePointer<AGAttributeType>
    ) -> UInt32 {
        return AGUnownedGraphRef.internAttributeType(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            type: type,
            makeAttributeType: makeAttributeType
        )
    }

}

extension String {

    static func _describing<Subject>(_ subject: UnsafeRawPointer, of type: Subject.Type) -> String {
        return String(describing: subject.assumingMemoryBound(to: Subject.self).pointee)
    }

}

extension AGAttributeType {

    nonisolated(unsafe) static let vtable: UnsafePointer<_AttributeVTable> = {
        let vtable = UnsafeMutablePointer<_AttributeVTable>.allocate(
            capacity: 1
        )
        vtable.pointee.version = 0
        vtable.pointee.type_destroy = { pointer in
            pointer.deallocate()
        }
        vtable.pointee.self_destroy = { attributeType, body in
            attributeType.pointee.attributeBody._destroySelf(body)
        }
        vtable.pointee.self_description = { attributeType, body in
            let description: String
            if let selfType = attributeType.pointee.self_id.type
                as? any CustomStringConvertible.Type
            {
                description = String._describing(body, of: selfType)
            } else {
                description = attributeType.pointee.self_id.description
            }
            return Unmanaged<CFString>.passRetained(description as NSString)
                .autorelease()
        }
        vtable.pointee.value_description = { attributeType, value in
            let valueType = attributeType.pointee.value_id.type
            let description = String._describing(value, of: valueType)
            return Unmanaged<CFString>.passRetained(description as NSString)
                .autorelease()
        }
        vtable.pointee.update_default = { attributeType, body in
            attributeType.pointee.attributeBody._updateDefault(body)
        }
        return UnsafePointer(vtable)
    }()

    @_extern(c, "AGRetainClosure")
    fileprivate static func passRetainedClosure(
        _ closure: (UnsafeMutableRawPointer, AnyAttribute) -> Void
    )
        -> AGClosureStorage

    init<Body: _AttributeBody>(
        selfType: Body.Type,
        valueType: Any.Type,
        flags: Flags,
        update: @escaping (UnsafeMutableRawPointer, AnyAttribute) -> Void,
    ) {
        var flags = flags
        flags.insert(selfType.flags)
        flags.insert(Flags(rawValue: UInt32(selfType.comparisonMode.rawValue)))
        if selfType._hasDestroySelf {
            flags.insert(.hasDestroySelf)
        }

        let retainedUpdate = AGAttributeType.passRetainedClosure(update)
        let conformance = unsafeBitCast(
            selfType as any _AttributeBody.Type,
            to: ProtocolConformance.self
        )
        self.init(
            self_id: Metadata(selfType),
            value_id: Metadata(valueType),
            update: retainedUpdate,
            vtable: AGAttributeType.vtable,
            flags: flags,
            internal_offset: 0,
            value_layout: nil,
            body_conformance: __Unnamed_struct_body_conformance(
                type_id: conformance.metadata,
                witness_table: conformance.witnessTable
            )
        )
    }

    var attributeBody: any _AttributeBody.Type {
        return unsafeBitCast(body_conformance, to: (any _AttributeBody.Type).self)
    }

}
