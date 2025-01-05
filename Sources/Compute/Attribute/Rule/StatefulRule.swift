import ComputeCxx

public protocol StatefulRule: _AttributeBody {

    associatedtype Value

    static var initialValue: Value? { get }
    func updateValue()

}

extension StatefulRule {

    public static var initialValue: Value? {
        return nil
    }

    public static func _updateDefault(_ default: UnsafeMutableRawPointer) {
        fatalError("not implemented")
    }
}

extension StatefulRule {

    public static func _update(_ body: UnsafeMutableRawPointer, attribute: AnyAttribute) {
        fatalError("not implemented")
    }

    public var bodyChanged: Bool {
        fatalError("not implemented")
    }

    public var value: Value {
        unsafeAddress {
            fatalError("not implemented")
        }
        set {
            fatalError("not implemented")
        }
    }

    public var hasValue: Bool {
        fatalError("not implemented")
    }

    public var attribute: Attribute<Value> {
        fatalError("not implemented")
    }
    
    public var context: RuleContext<Value> {
        fatalError("not implemented")
    }

}
