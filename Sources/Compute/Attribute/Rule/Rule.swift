import ComputeCxx

public protocol Rule: _AttributeBody {

    associatedtype Value

    static var initialValue: Value? { get }
    var value: Value { get }

}

extension Rule {

    public static var initialValue: Value? {
        return nil
    }

    public static func _updateDefault(_ self: UnsafeMutableRawPointer) {
        guard let initialValue = initialValue else {
            return
        }
        withUnsafePointer(to: initialValue) { initialValuePointer in
            Graph.setOutputValue(initialValuePointer)
        }
    }

    public static func _update(_ self: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        let rule = self.assumingMemoryBound(to: Self.self)
        let value = rule.pointee.value
        withUnsafePointer(to: value) { valuePointer in
            Graph.setOutputValue(valuePointer)
        }
    }

}

extension Rule {

    public var bodyChanged: Bool {
        fatalError("not implemented")
    }

    public var attribute: Attribute<Value> {
        return Attribute<Value>(identifier: AnyAttribute.current!)
    }

    public var context: RuleContext<Value> {
        return RuleContext<Value>(attribute: attribute)
    }

}

extension Graph {
    @_extern(c, "AGGraphReadCachedAttribute")
    static func readCachedAttribute(
        hash: Int,
        type: Metadata,
        body: UnsafeRawPointer,
        valueType: Metadata,
        options: CachedValueOptions,
        owner: AnyAttribute,
        changed: UnsafeMutablePointer<Bool>?,
        attributeTypeID: (AGUnownedGraphContextRef) -> UInt32
    ) -> UnsafeRawPointer
}

extension Rule where Self: Hashable {

    public func cachedValue(options: CachedValueOptions, owner: AnyAttribute?) -> Value {
        return withUnsafePointer(to: self) { bodyPointer in
            Self._cachedValue(
                options: options,
                owner: owner,
                hashValue: hashValue,
                bodyPtr: bodyPointer,
                update: { Self._update }
            ).pointee
        }
    }

    public func cachedValueIfExists(options: CachedValueOptions, owner: AnyAttribute?) -> Value? {
        return withUnsafePointer(to: self) { bodyPointer in
            let value = __AGGraphReadCachedAttributeIfExists(
                hashValue,
                Metadata(Self.self),
                bodyPointer,
                Metadata(Value.self),
                options,
                owner ?? .nil,
                nil
            )
            guard let value else {
                return nil
            }
            return value.assumingMemoryBound(to: Value.self).pointee
        }
    }

    public static func _cachedValue(
        options: CachedValueOptions,
        owner: AnyAttribute?,
        hashValue: Int,
        bodyPtr: UnsafeRawPointer,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UnsafePointer<Value> {
        let value = Graph.readCachedAttribute(
            hash: hashValue,
            type: Metadata(Self.self),
            body: bodyPtr,
            valueType: Metadata(Value.self),
            options: options,
            owner: owner ?? .nil,
            changed: nil
        ) { graph in
            return graph.internAttributeType(type: Metadata(Self.self)) {
                let attributeType = _AttributeType(
                    selfType: Self.self,
                    valueType: Value.self,
                    flags: [],  // TODO: check flags are empty
                    update: update()
                )
                let pointer = UnsafeMutablePointer<_AttributeType>.allocate(capacity: 1)
                pointer.initialize(to: attributeType)
                return UnsafePointer(pointer)
            }
        }
        return value.assumingMemoryBound(to: Value.self)
    }

}
