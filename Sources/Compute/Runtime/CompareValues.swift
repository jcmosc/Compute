public enum ComparisonMode {

}

public struct ComparisonOptions {
    
    init(mode: ComparisonMode) {
        fatalError("not implemented")
    }
    
}

func compareValues<Value>(_ lhs: Value, _ rhs: Value, mode: ComparisonMode) -> Bool {
    fatalError("not implemented")
}

func compareValues<Value>(_ lhs: Value, _ rhs: Value, mode: ComparisonOptions) -> Bool {
    fatalError("not implemented")
}


