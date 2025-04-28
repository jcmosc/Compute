import ComputeCxx

extension Subgraph {

    @_extern(c, "AGSubgraphAddObserver")
    private static func addObserver(_ subgraph: UnsafeRawPointer, observer: () -> Void) -> Int

    func addObserver(_ observer: () -> Void) -> Int {
        return withUnsafePointer(to: self) { selfPointer in
            return Subgraph.addObserver(selfPointer, observer: observer)
        }
    }

    public func apply<T>(_ body: () -> T) -> T {
        let previousSubgraph = Subgraph.current
        let previousUpdate = __AGGraphClearUpdate()
        defer {
            Subgraph.current = previousSubgraph
            __AGGraphSetUpdate(previousUpdate)
        }
        Subgraph.current = self
        return body()
    }

    @_extern(c, "AGSubgraphApply")
    private static func apply(_ subgraph: UnsafeRawPointer, flags: AGAttributeFlags, body: (AnyAttribute) -> Void)

    public func forEach(_ flags: AGAttributeFlags, _ body: (AnyAttribute) -> Void) {
        withUnsafePointer(to: self) { selfPointer in
            Subgraph.apply(selfPointer, flags: flags, body: body)
        }
    }

}

extension Subgraph {

    public static func addTreeValue<Value>(_ value: Attribute<Value>, forKey key: UnsafePointer<Int8>, flags: UInt32) {
        if shouldRecordTree {
            __AGSubgraphAddTreeValue(value.identifier, Metadata(Value.self), key, flags)
        }
    }

    public static func beginTreeElement<Value>(value: Attribute<Value>, flags: UInt32) {
        if shouldRecordTree {
            __AGSubgraphBeginTreeElement(value.identifier, Metadata(Value.self), flags)
        }
    }

    public static func endTreeElement<Value>(value: Attribute<Value>) {
        if shouldRecordTree {
            __AGSubgraphEndTreeElement(value.identifier)
        }
    }

}
