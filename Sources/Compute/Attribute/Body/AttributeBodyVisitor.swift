public protocol AttributeBodyVisitor {

    func visit<Body: _AttributeBody>(body: UnsafePointer<Body>)

}
