import Foundation

struct StatusResponse: Codable {
    let message: String
    let status: [String: Bool]
    let tempo: Double
    let deviceCount: Int
}

struct DevicesResponse: Codable {
    let devices: [Device]
    let count: Int
}

struct Device: Codable, Identifiable {
    let clientId: Int
    let boardId: String
    let ipAddress: String
    let lastStatusTime: UInt64

    var id: Int { clientId }

    var lastSeenText: String {
        let nowUs = UInt64(Date().timeIntervalSince1970 * 1_000_000)
        guard lastStatusTime > 0, nowUs > lastStatusTime else { return "unknown" }
        let elapsedSeconds = Double(nowUs - lastStatusTime) / 1_000_000
        if elapsedSeconds < 60 {
            return "\(Int(elapsedSeconds))s ago"
        } else if elapsedSeconds < 3600 {
            return "\(Int(elapsedSeconds / 60))m ago"
        } else {
            return "\(Int(elapsedSeconds / 3600))h ago"
        }
    }

    private enum CodingKeys: String, CodingKey {
        case clientId = "client_id"
        case boardId = "board_id"
        case ipAddress = "ip_address"
        case lastStatusTime = "last_status_time"
    }
}
