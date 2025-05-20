public struct Focus<Root, Value> {

    public var root: Attribute<Root>
    public var keyPath: KeyPath<Root, Value>

    public init(root: Attribute<Root>, keyPath: KeyPath<Root, Value>) {
        self.root = root
        self.keyPath = keyPath
    }

}

extension Focus: Rule {

    public var value: Value {
        return root.value[keyPath: keyPath]
    }

    public static var flags: AGAttributeTypeFlags {
        fatalError("not implemented")
    }

}

extension Focus: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}
