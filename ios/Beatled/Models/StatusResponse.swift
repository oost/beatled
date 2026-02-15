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
    let client_id: Int
    let board_id: String
    let ip_address: String
    let last_status_time: UInt64

    var id: Int { client_id }

    var lastSeenText: String {
        let nowUs = UInt64(Date().timeIntervalSince1970 * 1_000_000)
        guard last_status_time > 0, nowUs > last_status_time else { return "unknown" }
        let elapsedSeconds = Double(nowUs - last_status_time) / 1_000_000
        if elapsedSeconds < 60 {
            return "\(Int(elapsedSeconds))s ago"
        } else if elapsedSeconds < 3600 {
            return "\(Int(elapsedSeconds / 60))m ago"
        } else {
            return "\(Int(elapsedSeconds / 3600))h ago"
        }
    }
}
