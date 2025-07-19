import ComputeCxx

extension ComparisonOptions {

    public init(mode: ComparisonMode) {
        self.init(rawValue: UInt32(mode.rawValue))
    }

}

public func compareValues<Value>(_ lhs: Value, _ rhs: Value, mode: ComparisonMode = .equatableAlways) -> Bool {
    return compareValues(lhs, rhs, options: ComparisonOptions(mode: mode))
}

public func compareValues<Value>(_ lhs: Value, _ rhs: Value, options: ComparisonOptions) -> Bool {
    return withUnsafePointer(to: lhs) { lhsPointer in
        return withUnsafePointer(to: rhs) { rhsPointer in
            return __AGCompareValues(lhsPointer, rhsPointer, Metadata(Value.self), options.union(.copyOnWrite))
        }
    }
}
