import ComputeCxx

@_silgen_name("IAGSubgraphAddObserver")
func IAGSubgraphAddObserver(
    _ subgraph: UnsafeRawPointer,
    observer: () -> Void
) -> Int

extension Subgraph {
    public func addObserver(_ observer: @escaping () -> Void) -> Int {
        IAGSubgraphAddObserver(unsafeBitCast(self, to: UnsafeRawPointer.self), observer: observer)
    }
}

@_silgen_name("IAGSubgraphApply")
func IAGSubgraphApply(
    _ subgraph: UnsafeRawPointer,
    _ flags: Subgraph.Flags,
    _ body: (AnyAttribute) -> Void
)

extension Subgraph {

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

    public func forEach(
        _ flags: Subgraph.Flags,
        _ body: (AnyAttribute) -> Void
    ) {
        IAGSubgraphApply(unsafeBitCast(self, to: UnsafeRawPointer.self), flags, body)
    }

}

extension Subgraph {

    public static func beginTreeElement<Value>(
        value: Attribute<Value>,
        flags: UInt32
    ) {
        if shouldRecordTree {
            __IAGSubgraphBeginTreeElement(
                value.identifier,
                Metadata(Value.self),
                flags
            )
        }
    }

    public static func endTreeElement<Value>(value: Attribute<Value>) {
        if shouldRecordTree {
            __IAGSubgraphEndTreeElement(value.identifier)
        }
    }

    public static func addTreeValue<Value>(
        _ value: Attribute<Value>,
        forKey key: UnsafePointer<Int8>,
        flags: UInt32
    ) {
        if shouldRecordTree {
            __IAGSubgraphAddTreeValue(value.identifier, Metadata(Value.self), key, flags)
        }
    }

}
