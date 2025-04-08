import ComputeCxx

struct SubgraphContextVV {
    let body: () -> Void
}

struct SubgraphContextAV {
    let body: (AnyAttribute) -> Void
}

extension Subgraph {

    public func addObserver(_ observer: @escaping () -> Void) -> Int {
        let result = withUnsafePointer(to: SubgraphContextVV(body: observer)) { contextPointer in
            return __AGSubgraphAddObserver(
                self,
                {
                    $0.assumingMemoryBound(to: SubgraphContextVV.self).pointee.body()
                },
                contextPointer
            )
        }
        return Int(result)  // TODO: where is this converted?
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

    public func forEach(_ flags: AGAttributeFlags, _ body: (AnyAttribute) -> Void) {
        withoutActuallyEscaping(body) { escapingBody in
            withUnsafePointer(to: SubgraphContextAV(body: escapingBody)) { contextPointer in
                __AGSubgraphApply(
                    self,
                    flags,
                    {
                        // TODO: swap params
                        $1.assumingMemoryBound(to: SubgraphContextAV.self).pointee.body($0)
                    },
                    contextPointer
                )
            }
        }

    }

}

extension Subgraph {

    public static func addTreeValue<Value>(_ value: Attribute<Value>, forKey key: UnsafePointer<Int8>, flags: UInt32) {
        if __AGSubgraphShouldRecordTree() {
            __AGSubgraphAddTreeValue(value.identifier, Metadata(Value.self), key, flags)
        }
    }

    public static func beginTreeElement<Value>(value: Attribute<Value>, flags: UInt32) {
        if __AGSubgraphShouldRecordTree() {
            __AGSubgraphBeginTreeElement(value.identifier, Metadata(Value.self), flags)
        }
    }

    public static func endTreeElement<Value>(value: Attribute<Value>) {
        if __AGSubgraphShouldRecordTree() {
            __AGSubgraphEndTreeElement(value.identifier)
        }
    }

}
