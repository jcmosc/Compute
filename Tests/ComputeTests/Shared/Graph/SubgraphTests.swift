import Testing

@Suite
struct SubgraphTests {

    @Test
    func shouldRecordTree() {
        setenv("AG_TREE", "0", 1)
        #expect(Subgraph.shouldRecordTree == false)

        Subgraph.setShouldRecordTree()
        #expect(Subgraph.shouldRecordTree == true)
    }

    @Test
    func treeElementAPICheck() {
        let graph = Graph()
        let subgraph = Subgraph(graph: graph)
        subgraph.apply {
            let value = Attribute(value: ())
            Subgraph.beginTreeElement(value: value, flags: 0)
            Subgraph.addTreeValue(value, forKey: "", flags: 0)
            Subgraph.endTreeElement(value: value)
        }
    }

}
