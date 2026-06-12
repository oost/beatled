import Foundation

// POST /api/ap — switch the Pi's WiFi between client and access-point mode.
// `revertMinutes` only applies to mode "on" (auto-switch back after N minutes);
// it's optional, so the synthesized encoder omits it when nil.
struct APControlRequest: Encodable {
    let mode: String
    let revertMinutes: Int?
}

// Response to a "status" query.
struct APStatusResponse: Decodable {
    let ap: String // "on" or "off"
}

// Response to an "on"/"off" action.
struct APActionResponse: Decodable {
    let result: String
    let mode: String
    let output: String
}
