import ComputeCxx

extension TreeElement {

    public var value: AnyAttribute? {
        let result = __AGTreeElementGetValue(self)
        return result == .nil ? nil : result
    }

}

extension Nodes: @retroactive IteratorProtocol {
    public typealias Element = AnyAttribute

    public mutating func next() -> AnyAttribute? {
        let result = __AGTreeElementGetNextNode(&self)
        return result == .nil ? nil : result
    }

}

extension Children: @retroactive IteratorProtocol {
    public typealias Element = TreeElement
}

extension Values: @retroactive IteratorProtocol {
    public typealias Element = TreeValue
}

// TODO: how is this used?
extension TreeElement {

    struct LocalChildren {
        var base: Children
    }

}
