import ComputeCxx

extension TreeElement {
    public var value: AnyAttribute? {
        let result = __IAGTreeElementGetValue(self)
        return result == .nil ? nil : result
    }
}

extension TreeElement.Nodes: @retroactive Sequence, @retroactive IteratorProtocol {
    public typealias Element = AnyAttribute

    @_alwaysEmitIntoClient
    public mutating func next() -> AnyAttribute? {
        let result = __IAGTreeElementGetNextNode(&self)
        return result == .nil ? nil : result
    }
}

extension TreeElement.Children: @retroactive Sequence, @retroactive IteratorProtocol {
    public typealias Element = TreeElement
}

extension TreeElement.Values: @retroactive Sequence, @retroactive IteratorProtocol {
    public typealias Element = TreeValue
}

extension TreeElement {
    public struct LocalChildren: Sequence, IteratorProtocol {
        public var base: Children
        public mutating func next() -> TreeElement? {
            base.next(includeChildSubgraphs: false)
        }
    }
}
