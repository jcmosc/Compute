public struct Map<Arg, Value> {

    public var arg: Attribute<Arg>
    public var body: (Arg) -> Value

    public init(arg: Attribute<Arg>, body: @escaping (Arg) -> Value) {
        self.arg = arg
        self.body = body
    }

}

extension Map: Rule {

    public var value: Value {
        return body(arg.value)
    }

    public static var flags: AGAttributeTypeFlags {
        return []
    }

}

extension Map: CustomStringConvertible {

    public var description: String {
        return "λ \(Value.self)"
    }

}
