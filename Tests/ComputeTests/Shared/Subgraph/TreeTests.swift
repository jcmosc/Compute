import Testing

@Suite
final class TreeTests: GraphHost {

    @Test
    func shouldRecordTree() {
        #expect(Subgraph.shouldRecordTree == false)
        Subgraph.setShouldRecordTree()
        #expect(Subgraph.shouldRecordTree == true)
    }
    
    @Test
    func treeRoot() throws {
        struct TestRule: Rule {
            var value: String {
                return ""
            }
        }

        Subgraph.setShouldRecordTree()

        let attribute = Attribute(TestRule())

        Subgraph.beginTreeElement(value: attribute, flags: 1)
        Subgraph.endTreeElement(value: attribute)

        let treeRoot = try #require(Subgraph.current?.treeRoot)
        #expect(treeRoot.type == Metadata(String.self))
        #expect(treeRoot.value == attribute.identifier)
        #expect(treeRoot.flags == 1)
        #expect(treeRoot.parent == nil)
    }

    @Test
    func values() throws {
        struct TestRule: Rule {
            var value: String {
                return ""
            }
        }

        Subgraph.setShouldRecordTree()

        let attribute = Attribute(TestRule())
        let inputA = Attribute(value: "Input A")
        let inputB = Attribute(value: 100)

        Subgraph.beginTreeElement(value: attribute, flags: 1)
        Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
        Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)
        Subgraph.endTreeElement(value: attribute)

        let treeRoot = try #require(Subgraph.current?.treeRoot)
        var values = treeRoot.values

        let firstOrNil = values.next()
        let first = try #require(firstOrNil)
        #expect(first.type == Metadata(Int.self))
        #expect(String(cString: first.key) == "input_b")
        #expect(first.value == inputB.identifier)
        #expect(first.flags == 3)

        let secondOrNil = values.next()
        let second = try #require(secondOrNil)
        #expect(second.type == Metadata(String.self))
        #expect(String(cString: second.key) == "input_a")
        #expect(second.value == inputA.identifier)
        #expect(second.flags == 2)

        #expect(values.next() == nil)
    }

    @Test
    func children() throws {
        struct TestRule: Rule {
            var value: String {
                return ""
            }
        }

        Subgraph.setShouldRecordTree()

        let attribute = Attribute(TestRule())
        let inputA = Attribute(value: "Input A")
        let inputB = Attribute(value: 100)

        Subgraph.beginTreeElement(value: attribute, flags: 1)

        let childAttribute = Attribute(TestRule())
        let childInputA = Attribute(value: "Child Input A")
        let childInputB = Attribute(value: 200)

        Subgraph.beginTreeElement(value: childAttribute, flags: 11)
        Subgraph.addTreeValue(childInputA, forKey: "input_a", flags: 12)
        Subgraph.addTreeValue(childInputB, forKey: "input_b", flags: 13)
        Subgraph.endTreeElement(value: childAttribute)

        Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
        Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)
        Subgraph.endTreeElement(value: attribute)

        let treeRoot = try #require(Subgraph.current?.treeRoot)
        var children = treeRoot.children

        let firstOrNil = children.next()
        let first = try #require(firstOrNil)
        #expect(first.type == Metadata(String.self))
        #expect(first.value == childAttribute.identifier)
        #expect(first.flags == 11)
        #expect(first.parent == treeRoot)

        #expect(children.next() == nil)
    }

    @Test
    func nodes() throws {
        struct TestRule: Rule {
            var value: String {
                return ""
            }
        }

        Subgraph.setShouldRecordTree()

        let attribute = Attribute(TestRule())
        let inputA = Attribute(value: "Input A")
        let inputB = Attribute(value: 100)

        Subgraph.beginTreeElement(value: attribute, flags: 1)

        let childAttribute = Attribute(TestRule())
        let childInputA = Attribute(value: "Child Input A")
        let childInputB = Attribute(value: 200)

        Subgraph.beginTreeElement(value: childAttribute, flags: 1)
        Subgraph.addTreeValue(childInputA, forKey: "input_a", flags: 12)
        Subgraph.addTreeValue(childInputB, forKey: "input_b", flags: 13)
        Subgraph.endTreeElement(value: childAttribute)

        Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
        Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)
        Subgraph.endTreeElement(value: attribute)

        let treeRoot = try #require(Subgraph.current?.treeRoot)
        var nodes = treeRoot.nodes
        #expect(nodes.next() == childAttribute.identifier)
        #expect(nodes.next() == childInputA.identifier)
        #expect(nodes.next() == childInputB.identifier)
        #expect(nodes.next() == nil)
    }

}
