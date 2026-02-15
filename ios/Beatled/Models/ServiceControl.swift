import Foundation

struct ServiceControlRequest: Encodable {
    let id: String
    let status: Bool
}

struct ServiceControlResponse: Decodable {
    let status: Bool
}
