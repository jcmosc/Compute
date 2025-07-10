import ComputeCxx

extension Signature: @retroactive Equatable {

    public static func == (_ lhs: Signature, _ rhs: Signature) -> Bool {
        return lhs.bytes.0 == rhs.bytes.0 && lhs.bytes.1 == rhs.bytes.1
            && lhs.bytes.2 == rhs.bytes.2 && lhs.bytes.3 == rhs.bytes.3
            && lhs.bytes.4 == rhs.bytes.4 && lhs.bytes.5 == rhs.bytes.5
            && lhs.bytes.6 == rhs.bytes.6 && lhs.bytes.7 == rhs.bytes.7
            && lhs.bytes.8 == rhs.bytes.8 && lhs.bytes.9 == rhs.bytes.9
            && lhs.bytes.10 == rhs.bytes.10 && lhs.bytes.11 == rhs.bytes.11
            && lhs.bytes.12 == rhs.bytes.12 && lhs.bytes.13 == rhs.bytes.13
            && lhs.bytes.14 == rhs.bytes.14 && lhs.bytes.15 == rhs.bytes.15
            && lhs.bytes.16 == rhs.bytes.16 && lhs.bytes.17 == rhs.bytes.17
            && lhs.bytes.18 == rhs.bytes.18 && lhs.bytes.19 == rhs.bytes.19
    }

}
