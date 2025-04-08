import ComputeCxx

extension TreeElement {

    public var value: AnyAttribute? {
        let result = __AGTreeElementGetValue(self)
        return result == .nil ? nil : result
    }

}

extension Nodes: @retroactive Sequence, @retroactive IteratorProtocol {

    public mutating func next() -> AnyAttribute? {
        let result = __AGTreeElementGetNextNode(&self)
        return result == .nil ? nil : result
    }

}

extension Children: @retroactive Sequence, @retroactive IteratorProtocol {

    public mutating func next() -> TreeElement? {
        return __AGTreeElementGetNextChild(&self)
    }

}

extension Values: @retroactive Sequence, @retroactive IteratorProtocol {

    public func next() -> TreeValue? {
        return __AGTreeElementGetNextValue(self)
    }

}
