import ComputeCxx

extension Subgraph {

    public func addObserver(_ observer: () -> Void) -> Int {
        fatalError("not implemented")
    }

    public func apply<T>(_ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func forEach(_ flags: AttributeFlags, _ body: (AnyAttribute) -> Void) {
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
