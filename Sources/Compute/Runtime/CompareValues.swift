import ComputeCxx

extension AGComparisonOptions {

    public init(mode: AGComparisonMode) {
        self.init(rawValue: UInt32(mode.rawValue))
    }

}

public func compareValues<Value>(_ lhs: Value, _ rhs: Value, mode: AGComparisonMode = .equatableAlways) -> Bool {
    return compareValues(lhs, rhs, options: AGComparisonOptions(mode: mode))
}

public func compareValues<Value>(_ lhs: Value, _ rhs: Value, options: AGComparisonOptions) -> Bool {
    return withUnsafePointer(to: lhs) { lhsPointer in
        return withUnsafePointer(to: rhs) { rhsPointer in
            return __AGCompareValues(lhsPointer, rhsPointer, Metadata(Value.self), options.union(.copyOnWrite))
        }
    }
}
