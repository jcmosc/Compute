import ComputeCxx

extension Subgraph {

    @_extern(c, "AGSubgraphAddObserver")
    static func addObserver(_ subgraph: UnsafeRawPointer, observer: @escaping () -> Void) -> Int

    public func addObserver(_ observer: @escaping () -> Void) -> Int {
        return Subgraph.addObserver(unsafeBitCast(self, to: UnsafeRawPointer.self), observer: observer)
    }

    public func apply<T>(_ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func forEach(_ flags: AGAttributeFlags, _ body: (AnyAttribute) -> Void) {
        fatalError("not implemented")
    }

}

extension Subgraph {

    public func addTreeValue<Value>(_ attribute: Attribute<Value>, forKey key: UnsafePointer<Int8>, flags: UInt32) {
        fatalError("not implemented")
    }

    public func beginTreeElement<Value>(value: Attribute<Value>, flags: UInt32) {
        fatalError("not implemented")
    }

    public func endTreeElement<Value>(value: Attribute<Value>) {
        fatalError("not implemented")
    }

}
