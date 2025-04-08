import ComputeCxx

public protocol Rule: _AttributeBody {

    associatedtype Value

    var value: Value { get }
    static var initialValue: Value? { get }

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
            __AGGraphSetOutputValue(initialValuePointer, Metadata(Value.self))
        }
    }

    public static func _update(_ pointer: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        let rule = pointer.assumingMemoryBound(to: Self.self)
        let value = rule.pointee.value
        withUnsafePointer(to: value) { valuePointer in
            __AGGraphSetOutputValue(valuePointer, Metadata(Value.self))
        }
    }

}

extension Rule {

    public var bodyChanged: Bool {
        // guessing this is it
        return __AGGraphCurrentAttributeWasModified()
    }

    public var attribute: Attribute<Value> {
        guard let attribute = AnyAttribute.current else {
            preconditionFailure()
        }
        return Attribute<Value>(identifier: attribute)
    }

    public var context: RuleContext<Value> {
        return RuleContext<Value>(attribute: attribute)
    }

}

private struct RuleCachedValueContext {}

//  AutoClass1::FUN_1afe81fc
func makeSomething(
    graph: Graph,
    update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void,
    selfType: Any.Type,
    bodyType: _AttributeBody.Type,
    valueType: Any.Type
) -> UInt32 {
    return graph.internAttributeType(
        selfType: selfType,
        bodyType: bodyType,
        valueType: valueType,
        flags: [],
        update: update
    )
}

extension Rule where Self: Hashable {

    public func cachedValue(options: AGCachedValueOptions, owner: AnyAttribute?) -> Value {
        return withUnsafePointer(to: self) { selfPointer in
            let result = Self._cachedValue(options: options, owner: owner, hashValue: hashValue, bodyPtr: selfPointer) {
                return Self._update(_:attribute:)
            }
            return result.pointee
        }
    }

    public func cachedValueIfExists(options: AGCachedValueOptions, owner: AnyAttribute?) -> Value? {
        let result = withUnsafePointer(to: self) { selfPointer in
            return __AGGraphReadCachedAttributeIfExists(
                UInt64(hashValue),  // TODO: bitPattern?
                Metadata(Self.self),
                UnsafeMutablePointer(mutating: selfPointer),  // TODO: need to mutate this? make const at C and propogate...
                Metadata(Value.self),
                options,
                owner ?? .nil,
                nil
            )
        }
        guard let result = result else {
            return nil
        }
        return result.assumingMemoryBound(to: Value.self).pointee
    }

    public static func _cachedValue(
        options: AGCachedValueOptions,
        owner: AnyAttribute?,
        hashValue: Int,
        bodyPtr: UnsafeRawPointer,
        update: () -> (UnsafeMutableRawPointer, AnyAttribute) -> Void
    ) -> UnsafePointer<Value> {
        let result = withUnsafePointer(to: self) { selfPointer in
            var context = RuleCachedValueContext()
            return __AGGraphReadCachedAttribute(
                UInt64(hashValue),  // TODO: bitPattern?
                Metadata(Self.self),
                UnsafeMutablePointer(mutating: selfPointer),
                Metadata(Value.self),
                options,
                owner ?? .nil,
                nil,
                {
                    graph,
                    context in

                    // TODO: fix as! Graph
                    //                    return makeSomething(graph: graph as! Graph, update: update, selfType: Self.self, bodyType: Self.self, valueType: Value.self)
                    return 0
                },
                &context
            )
        }
        let pointer = result.assumingMemoryBound(to: Value.self)
        return UnsafePointer(pointer)
    }

}
