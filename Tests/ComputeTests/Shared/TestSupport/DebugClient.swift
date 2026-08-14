import Foundation
import Network

@available(macOS 26, iOS 26, tvOS 26, watchOS 26, *)
struct DebugClient {
    private let endpoint: NWEndpoint
    private let token: UInt32

    init?(url: URL) {
        guard
            let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
            let host = components.host,
            let port = components.port,
            let queryItems = components.queryItems,
            let tokenItem = queryItems.first(where: { $0.name == "token" }),
            let tokenValue = tokenItem.value,
            let token = UInt32(tokenValue),
            let nwPort = NWEndpoint.Port(rawValue: UInt16(port))
        else {
            return nil
        }
        self.endpoint = .hostPort(host: NWEndpoint.Host(host), port: nwPort)
        self.token = token
    }

    func performCommand(_ command: String, data: [String: Any] = [:]) async throws -> Any? {
        var requestBody: [String: Any] = [
            "command": command
        ]
        for (key, value) in data {
            requestBody[key] = value
        }

        let requestBodyData = try JSONSerialization.data(withJSONObject: requestBody)
        let requestHeader = DebugServer.MessageHeader(
            token: self.token,
            reserved1: 0,
            body_length: UInt32(requestBodyData.count),
            reserved2: 0
        )
        let requestHeaderData = withUnsafePointer(to: requestHeader) { pointer in
            Data(bytes: UnsafeRawPointer(pointer), count: DebugServer.MessageHeader.size)
        }

        var response: Any?
        try await withNetworkConnection(to: endpoint, using: { TCP() }) { connection in
            try await connection.send(requestHeaderData)
            try await connection.send(requestBodyData)

            let responseHeaderMessage = try await connection.receive(exactly: DebugServer.MessageHeader.size)
            let responseHeader = responseHeaderMessage.content.withUnsafeBytes { pointer in
                pointer.baseAddress!.assumingMemoryBound(to: DebugServer.MessageHeader.self).pointee
            }
            precondition(responseHeader.token == token, "token mismatch")

            guard responseHeader.body_length > 0 else {
                return
            }
            let responseBodyMessage = try await connection.receive(exactly: Int(responseHeader.body_length))
            response = try JSONSerialization.jsonObject(with: responseBodyMessage.content)
        }
        return response
    }
}

extension DebugServer.MessageHeader {
    fileprivate static var size: Int {
        MemoryLayout<DebugServer.MessageHeader>.size
    }
}
