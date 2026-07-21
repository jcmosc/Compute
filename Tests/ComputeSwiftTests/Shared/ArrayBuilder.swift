@resultBuilder
struct ArrayBuilder<Element> {
    static func buildPartialBlock(first: Element) -> [Element] {
        [first]
    }

    static func buildPartialBlock(accumulated: [Element], next: Element) -> [Element] {
        var result = accumulated
        result.append(next)
        return result
    }
}

extension Array {
    init(@ArrayBuilder<Element> build: () -> [Element]) {
        self = build()
    }
}
