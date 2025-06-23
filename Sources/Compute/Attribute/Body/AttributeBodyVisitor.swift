public protocol AttributeBodyVisitor {

    mutating func visit<Body: _AttributeBody>(body: UnsafePointer<Body>)

}

extension _AttributeBody {

    static func _visitSelf<Visitor: AttributeBodyVisitor>(_ self: UnsafeRawPointer, visitor: inout Visitor) {
        visitor.visit(body: self.assumingMemoryBound(to: Self.self))
    }

}
