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
        let update = Graph.clearUpdate()
        let current = Subgraph.current
        defer {
            Subgraph.current = current
            Graph.setUpdate(update)
        }
        Subgraph.current = self
        return body()
    }

    @_extern(c, "AGSubgraphApply")
    private static func forEach(
        _ subgraph: UnsafeRawPointer,
        _ flags: Subgraph.Flags,
        _ body: (AnyAttribute) -> Void
    )

    public func forEach(
        _ flags: Subgraph.Flags,
        _ body: (AnyAttribute) -> Void
    ) {
        Subgraph.forEach(unsafeBitCast(self, to: UnsafeRawPointer.self), flags, body)
    }

}

extension Subgraph {

    public static func beginTreeElement<Value>(
        value: Attribute<Value>,
        flags: UInt32
    ) {
        if shouldRecordTree {
            __AGSubgraphBeginTreeElement(
                value.identifier,
                Metadata(Value.self),
                flags
            )
        }
    }

    public static func endTreeElement<Value>(value: Attribute<Value>) {
        if shouldRecordTree {
            __AGSubgraphEndTreeElement(value.identifier)
        }
    }

    public static func addTreeValue<Value>(
        _ value: Attribute<Value>,
        forKey key: UnsafePointer<Int8>,
        flags: UInt32
    ) {
        if shouldRecordTree {
            __AGSubgraphAddTreeValue(value.identifier, Metadata(Value.self), key, flags)
        }
    }

}
