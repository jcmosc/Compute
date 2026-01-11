import Foundation

extension Graph {
    public struct DictionaryDescription: Equatable, Codable {

        public struct Counters: Equatable, Codable {
            public var bytes: Int
            public var maxBytes: Int
            public enum CodingKeys: String, CodingKey {
                case bytes
                case maxBytes = "max_bytes"
            }
        }

        public struct Graph: Equatable, Codable {

            public struct Counters: Equatable, Codable {
                public var nodes: Int
                public var createdNodes: Int
                public var maxNodes: Int
                public var subgraphs: Int
                public var createdSubgraphs: Int
                public var maxSubgraphs: Int
                public var updates: Int
                public var changes: Int
                public var transactions: Int
                public enum CodingKeys: String, CodingKey {
                    case nodes
                    case createdNodes = "created_nodes"
                    case maxNodes = "max_nodes"
                    case subgraphs
                    case createdSubgraphs = "created_subgraphs"
                    case maxSubgraphs = "max_subgraphs"
                    case updates
                    case changes
                    case transactions
                }
            }

            public struct Edge: Equatable, Codable {
                public var source: Int
                public var destination: Int
                public var offset: Int?
                public var flags: Int?
                public enum CodingKeys: String, CodingKey {
                    case source = "src"
                    case destination = "dst"
                    case offset
                    case flags
                }
            }

            public struct Node: Equatable, Codable {
                public var id: Int
                public var type: Int
                public var flags: Int
                public var description: String
                public var subgraphFlags: Int?
                public var value: String?
                public enum CodingKeys: String, CodingKey {
                    case id
                    case type
                    case flags
                    case description = "desc"
                    case subgraphFlags = "subgraph_flags"
                    case value
                }
            }

            public struct Subgraph: Equatable, Codable {
                public var id: Int
                public var contextID: Int
                public var nodes: [Int]?
                public var parents: [Int]?
                public var children: [Int]?
                public enum CodingKeys: String, CodingKey {
                    case id
                    case contextID = "context_id"
                    case nodes
                    case parents
                    case children
                }
                public init(id: Int, contextID: Int, nodes: [Int]? = nil, parents: [Int]? = nil, children: [Int]? = nil)
                {
                    self.id = id
                    self.contextID = contextID
                    self.nodes = nodes
                    self.parents = parents
                    self.children = children
                }
            }

            public struct Tree: Equatable, Codable {
                public struct Value: Equatable, Codable {
                    public var node: Int
                    public var offset: Int?
                    public enum CodingKeys: String, CodingKey {
                        case node
                        case offset
                    }
                }

                public var node: Int?
                public var offset: Int?
                public var description: String?
                public var root: Bool?
                public var flags: Int?
                public var children: [Int]?
                public var nodes: [Int]?
                public var values: [String: Value]?
                public enum CodingKeys: String, CodingKey {
                    case node
                    case offset
                    case description = "desc"
                    case root
                    case flags
                    case children
                    case nodes
                    case values
                }
            }

            public struct AttributeType: Equatable, Codable {
                public var id: Int
                public var name: String
                public var flags: Int
                public var size: Int
                public var value: String
                public enum CodingKeys: String, CodingKey {
                    case id
                    case name
                    case flags
                    case size
                    case value
                }
            }

            public var id: Int
            public var counters: Counters
            public var types: [AttributeType]
            public var nodes: [Node]
            public var edges: [Edge]
            public var subgraphs: [Subgraph]
            public var trees: [Tree]?
            public var updateCount: Int
            public var changeCount: Int
            public var transactionCount: Int
            public enum CodingKeys: String, CodingKey {
                case id
                case counters
                case types
                case nodes
                case edges
                case subgraphs
                case trees
                case updateCount = "update_count"
                case changeCount = "change_count"
                case transactionCount = "transaction_count"
            }
        }

        public var version: Int
        public var counters: Counters
        public var graphs: [Graph]
        public enum CodingKeys: String, CodingKey {
            case version
            case counters
            case graphs
        }

    }

    public func dictionaryDescription(includeValues: Bool = false) -> DictionaryDescription? {
        let options =
            [DescriptionOption.format: "graph/dict", DescriptionOption.includeValues: includeValues] as NSDictionary
        guard let description = Graph.description(self, options: options) as? NSDictionary else {
            return nil
        }
        let data = try! JSONSerialization.data(withJSONObject: description, options: [.prettyPrinted, .sortedKeys])
        return try! JSONDecoder().decode(DictionaryDescription.self, from: data)
    }

    public static func dictionaryDescription(
        allGraphs: Bool = false,
        includeValues: Bool = false
    )
        -> DictionaryDescription?
    {
        let options =
            [
                DescriptionOption.format: "graph/dict", DescriptionOption.includeValues: includeValues,
                "all_graphs": allGraphs,
            ] as NSDictionary
        guard let description = Graph.description(nil, options: options) as? NSDictionary else {
            return nil
        }
        guard
            let data = try? JSONSerialization.data(withJSONObject: description, options: [.prettyPrinted, .sortedKeys])
        else {
            return nil
        }
        return try? JSONDecoder().decode(DictionaryDescription.self, from: data)
    }

    public static func dictionaryDescriptionJSON(
        allGraphs: Bool = false,
        includeValues: Bool = false
    )
        -> Data?
    {
        let options =
            [
                DescriptionOption.format: "graph/dict", DescriptionOption.includeValues: includeValues,
                "all_graphs": allGraphs,
            ] as NSDictionary
        guard let description = Graph.description(nil, options: options) as? NSDictionary else {
            return nil
        }
        return try? JSONSerialization.data(withJSONObject: description, options: [.prettyPrinted, .sortedKeys])
    }

}
