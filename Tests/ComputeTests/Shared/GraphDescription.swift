struct GraphDescription: Decodable {
    
    struct Graph: Decodable {
        var nodes: [Node]
    }
    
    struct Node: Decodable {
        var type: Int
        var id: Int
        var desc: String?
        var value: String?
        var flags: Int?
    }
    
    var graphs: [Graph]
    var version: Int
    
}

