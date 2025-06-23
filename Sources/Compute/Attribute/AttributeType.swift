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
    func internAttributeType(type: Metadata, makeAttributeType: () -> UnsafePointer<AGAttributeType>) -> UInt32 {
        return AGUnownedGraphRef.internAttributeType(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            type: type,
            makeAttributeType: makeAttributeType
        )
    }

}

extension CustomStringConvertible {

    static func _description(for self: UnsafeRawPointer) -> String {
        return self.assumingMemoryBound(to: Self.self).pointee.description
    }

}

extension AGAttributeType {

    nonisolated(unsafe) static let callbacks: UnsafePointer<AGAttributeVTable> = {
        let callbacks = UnsafeMutablePointer<AGAttributeVTable>.allocate(capacity: 1)
        callbacks.pointee.allocate = nil
        callbacks.pointee.deallocate = { pointer in
            pointer.deallocate()
        }
        callbacks.pointee.destroySelf = { attributeType, body in
            attributeType.pointee.attributeBody._destroySelf(body)
        }
        callbacks.pointee.bodyDescription = { attributeType, body in
            let description: String
            if let selfType = attributeType.pointee.selfType.type as? any CustomStringConvertible.Type {
                description = selfType._description(for: body)
            } else {
                description = attributeType.pointee.selfType.description
            }
            return Unmanaged<CFString>.passRetained(description as NSString).autorelease()
        }
        callbacks.pointee.valueDescription = { attributeType, value in
            let description: String
            if let valueType = attributeType.pointee.valueType.type as? any CustomStringConvertible.Type {
                description = valueType._description(for: value)
            } else {
                description = attributeType.pointee.valueType.description
            }
            return Unmanaged<CFString>.passRetained(description as NSString).autorelease()
        }
        callbacks.pointee.initializeValue = { _, _ in
            fatalError("not implemented")
        }
        return UnsafePointer(callbacks)
    }()

    init<Body: _AttributeBody>(
        selfType: Body.Type,
        valueType: Any.Type,
        flags: AGAttributeTypeFlags,
        update: @escaping (UnsafeMutableRawPointer, AnyAttribute) -> Void,
    ) {
        var flags = flags
        flags.insert(selfType.flags)
        flags.insert(AGAttributeTypeFlags(rawValue: UInt32(selfType.comparisonMode.rawValue)))
        if selfType._hasDestroySelf {
            flags.insert(.hasDestroySelf)
        }

        let conformance = unsafeBitCast(selfType as any _AttributeBody.Type, to: ProtocolConformance.self)
        self.init(
            selfType: Metadata(selfType),
            valueType: Metadata(valueType),
            update: unsafeBitCast(update, to: AGClosureStorage.self),
            callbacks: AGAttributeType.callbacks,
            flags: flags,
            selfOffset: 0,
            layout: nil,
            attributeBodyType: conformance.metadata,
            attributeBodyWitnessTable: conformance.witnessTable
        )
    }

    var attributeBody: any _AttributeBody.Type {
        let conformance = ProtocolConformance(metadata: attributeBodyType, witnessTable: attributeBodyWitnessTable)
        return unsafeBitCast(conformance, to: (any _AttributeBody.Type).self)
    }

}
