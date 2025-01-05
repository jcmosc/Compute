public struct TreeElement {}

extension TreeElement {

    public var value: AnyAttribute? {
        fatalError("not implemented")
    }

}

struct Nodes: Sequence, IteratorProtocol {

    public func next() -> AnyAttribute? {
        fatalError("not implemented")
    }

}

struct Children: Sequence, IteratorProtocol {

    public mutating func next() -> TreeElement? {
        fatalError("not implemented")
    }

}

struct Values: Sequence, IteratorProtocol {

    public func next() -> TreeElement? {
        fatalError("not implemented")
    }

}
