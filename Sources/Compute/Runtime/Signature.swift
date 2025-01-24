import ComputeCxx

extension Signature: @retroactive Equatable {

    public static func == (_ lhs: Signature, _ rhs: Signature) -> Bool {
        return lhs.data.0 == rhs.data.0 && lhs.data.1 == rhs.data.1 && lhs.data.2 == rhs.data.2
            && lhs.data.3 == rhs.data.3 && lhs.data.4 == rhs.data.4
    }

}
