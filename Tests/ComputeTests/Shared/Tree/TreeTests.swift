import Testing

// does endTree clear tree if it is the root?
// create subgraph while update or no update
// create subgraph with owner attribute

@Suite(.serialized(for: \Subgraph.Type.current))
struct TreeTests {
    @Test
    func shouldRecordTree() async throws {
        try await #require(processExitsWith: .success) {
            try #require(Subgraph.shouldRecordTree == false)

            Subgraph.setShouldRecordTree()
            #expect(Subgraph.shouldRecordTree == true)
        }
    }

    @Test
    func subgraphInitialization() async throws {
        try await #require(processExitsWith: .success) {
            try #require(Subgraph.shouldRecordTree == false)

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.treeRoot == nil)
        }
        try await #require(processExitsWith: .success) {
            Subgraph.setShouldRecordTree()
            try #require(Subgraph.shouldRecordTree == true)

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            let treeRoot = try #require(subgraph.treeRoot)
            #expect(treeRoot.type.rawValue == UnsafePointer(bitPattern: 0))
            #expect(treeRoot.value == nil)
            #expect(treeRoot.flags == 0)
            #expect(treeRoot.parent == nil)
        }
    }

    @Test
    func treeRoot() throws {
        struct TestRule: Rule {
            var value: String {
                return ""
            }
        }

        Subgraph.setShouldRecordTree()

        try withGraph {
            let originalTreeRoot = Subgraph.current?.treeRoot

            let attribute = Attribute(TestRule())

            Subgraph.beginTreeElement(value: attribute, flags: 1)
            defer {
                Subgraph.endTreeElement(value: attribute)
            }

            let treeRoot = try #require(Subgraph.current?.treeRoot)
            #expect(treeRoot.type == Metadata(String.self))
            #expect(treeRoot.value == attribute.identifier)
            #expect(treeRoot.flags == 1)
            #expect(treeRoot.parent == originalTreeRoot)
        }
    }
    
    @Suite
    struct ChildrenTests {
        @Test
        func children() throws {
            struct TestRule: Rule {
                var value: String {
                    return ""
                }
            }

            Subgraph.setShouldRecordTree()

            try withGraph {
                let attribute = Attribute(TestRule())
                let inputA = Attribute(value: "Input A")
                let inputB = Attribute(value: 100)

                Subgraph.beginTreeElement(value: attribute, flags: 1)
                defer {
                    Subgraph.endTreeElement(value: attribute)
                }

                let childAttribute = Attribute(TestRule())
                let childInputA = Attribute(value: "Child Input A")
                let childInputB = Attribute(value: 200)

                Subgraph.beginTreeElement(value: childAttribute, flags: 11)
                Subgraph.addTreeValue(childInputA, forKey: "input_a", flags: 12)
                Subgraph.addTreeValue(childInputB, forKey: "input_b", flags: 13)
                Subgraph.endTreeElement(value: childAttribute)

                Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
                Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)

                let treeRoot = try #require(Subgraph.current?.treeRoot)
                let children = Array(treeRoot.children)
                try #require(children.count == 1)

                #expect(children[0].type == Metadata(String.self))
                #expect(children[0].value == childAttribute.identifier)
                #expect(children[0].flags == 11)
                #expect(children[0].parent == treeRoot)
            }
        }

        @Test(.recordTree)
        func childrenTraversingChildSubgraphs() throws {
            struct TestRule: Rule {
                var value: String {
                    return ""
                }
            }

            var keepAlivePool: [Subgraph] = []

            Subgraph.setShouldRecordTree()

            try withGraph {
                Subgraph.current!.index = 100

                let attribute = Attribute(TestRule())
                var subgraphOwner: Attribute<String>!
                var childAttribute1: Attribute<String>!
                var childAttribute2: Attribute<String>!

                makeTreeElement(attribute, flags: 1) {
                    let childSubgraph = Subgraph(graph: Subgraph.current!.graph)
                    Subgraph.current!.addChild(childSubgraph)
                    childSubgraph.index = 200
                    keepAlivePool.append(childSubgraph)

                    // Link child subgraph to parent subgraph tree
                    subgraphOwner = Attribute(TestRule())
                    childSubgraph.setTreeOwner(subgraphOwner.identifier)

                    childSubgraph.apply {
                        childAttribute1 = Attribute(TestRule())
                        makeTreeElement(childAttribute1, flags: 2) {
                            // empty
                        }

                        childAttribute2 = Attribute(TestRule())
                        makeTreeElement(childAttribute2, flags: 3) {
                            // empty
                        }
                    }
                }

                let treeRoot = try #require(Subgraph.current?.treeRoot)
                #expect(
                    treeRoot.debugDescription == """
                        (tree
                          (element
                            (element #:type String #:value \(attribute) #:flags 1
                              (element #:value \(subgraphOwner!)
                                (element #:type String #:value \(childAttribute2!) #:flags 3)
                                (element #:type String #:value \(childAttribute1!) #:flags 2)))))
                        """
                )

                keepAlivePool.removeAll()
            }
        }
    }
    
    @Suite
    struct ValuesTests {
        @Test
        func values() async throws {
            struct TestRule: Rule {
                var value: String {
                    return ""
                }
            }

            Subgraph.setShouldRecordTree()

            try withGraph {
                let attribute = Attribute(TestRule())
                let inputA = Attribute(value: "Input A")
                let inputB = Attribute(value: 100)

                Subgraph.beginTreeElement(value: attribute, flags: 1)
                defer {
                    Subgraph.endTreeElement(value: attribute)
                }

                Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
                Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)

                let treeRoot = try #require(Subgraph.current?.treeRoot)
                #expect(treeRoot.value == attribute.identifier)

                let values = Array(treeRoot.values)
                try #require(values.count == 2)

                #expect(values[0].type == Metadata(Int.self))
                #expect(String(cString: values[0].key) == "input_b")
                #expect(values[0].value == inputB.identifier)
                #expect(values[0].flags == 3)

                #expect(values[1].type == Metadata(String.self))
                #expect(String(cString: values[1].key) == "input_a")
                #expect(values[1].value == inputA.identifier)
                #expect(values[1].flags == 2)
            }
        }
    }
    
    @Suite
    struct NodesTests {
        @Test
        func nodes() throws {
            struct TestRule: Rule {
                var value: String {
                    return ""
                }
            }

            Subgraph.setShouldRecordTree()

            try withGraph {
                let attribute = Attribute(TestRule())
                let inputA = Attribute(value: "Input A")
                let inputB = Attribute(value: 100)

                Subgraph.beginTreeElement(value: attribute, flags: 1)
                defer {
                    Subgraph.endTreeElement(value: attribute)
                }

                let childAttribute = Attribute(TestRule())
                let childInputA = Attribute(value: "Child Input A")
                let childInputB = Attribute(value: 200)

                Subgraph.beginTreeElement(value: childAttribute, flags: 1)
                Subgraph.addTreeValue(childInputA, forKey: "input_a", flags: 12)
                Subgraph.addTreeValue(childInputB, forKey: "input_b", flags: 13)
                Subgraph.endTreeElement(value: childAttribute)

                Subgraph.addTreeValue(inputA, forKey: "input_a", flags: 2)
                Subgraph.addTreeValue(inputB, forKey: "input_b", flags: 3)

                let treeRoot = try #require(Subgraph.current?.treeRoot)
                let nodes = Array(treeRoot.nodes)
                try #require(nodes.count == 3)
                
                #expect(nodes[0] == childAttribute.identifier)
                #expect(nodes[1] == childInputA.identifier)
                #expect(nodes[2] == childInputB.identifier)
            }
        }
    }
}

func makeTreeElement<T, U>(_ attribute: Attribute<T>, flags: UInt32, body: () -> U) -> U {
    Subgraph.beginTreeElement(value: attribute, flags: flags)
    defer {
        Subgraph.endTreeElement(value: attribute)
    }

    return body()
}
