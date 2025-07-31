import ComputeCxx

extension Subgraph {

    @_extern(c, "AGSubgraphAddObserver")
    static func addObserver(
        _ subgraph: UnsafeRawPointer,
        observer: @escaping () -> Void
    ) -> Int

    public func addObserver(_ observer: @escaping () -> Void) -> Int {
        return Subgraph.addObserver(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            observer: observer
        )
    }

    public func apply<T>(_ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func forEach(
        _ flags: Subgraph.Flags,
        _ body: (AnyAttribute) -> Void
    ) {
        fatalError("not implemented")
    }

}

extension Subgraph {

    public static func beginTreeElement<Value>(
        value: Attribute<Value>,
        flags: UInt32
    ) {
        Subgraph.beginTreeElement(
            value: value.identifier,
            type: Metadata(Value.self),
            flags: flags
        )
    }

    public static func endTreeElement<Value>(value: Attribute<Value>) {
        Subgraph.endTreeElement(value: value.identifier)
    }

    public static func addTreeValue<Value>(
        _ value: Attribute<Value>,
        forKey key: UnsafePointer<Int8>,
        flags: UInt32
    ) {
        Subgraph.addTreeValue(value: value.identifier, type: Metadata(Value.self), forKey: key, flags: flags)
    }

}
