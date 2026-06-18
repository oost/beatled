import Foundation

struct StatusResponse: Codable {
    let message: String
    let status: [String: Bool]
    let tempo: Double
    let deviceCount: Int
    // Optional so older servers without the field still decode.
    let manualBpm: Double?
    // Microseconds since the server process started (time since last restart).
    // Optional so older servers without the field still decode.
    let uptimeUs: Double?

    private enum CodingKeys: String, CodingKey {
        case message
        case status
        case tempo
        case deviceCount
        case manualBpm
        case uptimeUs = "uptime_us"
    }
}

// Compact duration: "3d 4h", "4h 12m", "12m 5s", "5s". Shared by the server
// uptime row and the per-device uptime column.
func formatDurationUs(_ us: Double) -> String {
    let totalSec = Int(us / 1_000_000)
    let d = totalSec / 86400
    let h = (totalSec % 86400) / 3600
    let m = (totalSec % 3600) / 60
    let s = totalSec % 60
    if d > 0 { return "\(d)d \(h)h" }
    if h > 0 { return "\(h)h \(m)m" }
    if m > 0 { return "\(m)m \(s)s" }
    return "\(s)s"
}

struct DevicesResponse: Codable {
    let devices: [Device]
    let count: Int
}

// Protocol v4 per-device diagnostic snapshot carried in /api/devices.
// Every field is optional so abbreviated or older payloads still decode;
// the UI renders missing values as "—".
struct DeviceQos: Codable {
    let currentOffsetUs: Double?
    let uptimeUs: Double?
    let medianRttUs: Double?
    let nextBeatGapTotal: Int?
    let intercoreDropTotal: Int?
    let timeSyncOutlierTotal: Int?
    let validSampleCount: Int?
    let lastAppliedProgramSeq: Int?
    let serverReceivedAtUs: Double?
    let lastRttUs: Double?
    let syncErrorUs: Double?

    private enum CodingKeys: String, CodingKey {
        case currentOffsetUs = "current_offset_us"
        case uptimeUs = "uptime_us"
        case medianRttUs = "median_rtt_us"
        case nextBeatGapTotal = "next_beat_gap_total"
        case intercoreDropTotal = "intercore_drop_total"
        case timeSyncOutlierTotal = "time_sync_outlier_total"
        case validSampleCount = "valid_sample_count"
        case lastAppliedProgramSeq = "last_applied_program_seq"
        case serverReceivedAtUs = "server_received_at_us"
        case lastRttUs = "last_rtt_us"
        case syncErrorUs = "sync_error_us"
    }
}

// Fleet-wide aggregates from /api/qos. Numeric fields are null until at
// least one device has reported a v4 QoS snapshot.
struct FleetQos: Codable {
    struct Thresholds: Codable {
        let skewWarnUs: Double
        let skewFailUs: Double

        private enum CodingKeys: String, CodingKey {
            case skewWarnUs = "skew_warn_us"
            case skewFailUs = "skew_fail_us"
        }
    }

    let deviceCount: Int
    let reportingCount: Int
    let minOffsetUs: Double?
    let maxOffsetUs: Double?
    let fleetSkewUs: Double?
    let meanRttUs: Double?
    let minRttUs: Double?
    let maxRttUs: Double?
    let slowestDeviceBoardId: String
    let totalNextBeatGap: Int
    let totalIntercoreDrops: Int
    let totalTimeSyncOutliers: Int
    // Kept as a raw string ("ok" / "warn" / "fail" / "unknown") so a future
    // server-side verdict doesn't break decoding; the view maps known values.
    let health: String
    let thresholds: Thresholds

    private enum CodingKeys: String, CodingKey {
        case deviceCount = "device_count"
        case reportingCount = "reporting_count"
        case minOffsetUs = "min_offset_us"
        case maxOffsetUs = "max_offset_us"
        case fleetSkewUs = "fleet_skew_us"
        case meanRttUs = "mean_rtt_us"
        case minRttUs = "min_rtt_us"
        case maxRttUs = "max_rtt_us"
        case slowestDeviceBoardId = "slowest_device_board_id"
        case totalNextBeatGap = "total_next_beat_gap"
        case totalIntercoreDrops = "total_intercore_drops"
        case totalTimeSyncOutliers = "total_time_sync_outliers"
        case health
        case thresholds
    }
}

struct Device: Codable, Identifiable {
    let clientId: Int
    let boardId: String
    let ipAddress: String
    let lastStatusTime: UInt64
    // v3+ firmware self-description; nil/empty for older clients.
    let portName: String?
    let gitSha: String?
    let buildTimeUs: UInt64?
    // Server-smoothed one-way delay estimate; diagnostic only.
    let owdUs: Double?
    // v4 QoS snapshot; nil until the device's first TEMPO_REQUEST or
    // STATUS_RESPONSE.
    let qos: DeviceQos?

    init(clientId: Int, boardId: String, ipAddress: String, lastStatusTime: UInt64,
         portName: String? = nil, gitSha: String? = nil, buildTimeUs: UInt64? = nil,
         owdUs: Double? = nil, qos: DeviceQos? = nil) {
        self.clientId = clientId
        self.boardId = boardId
        self.ipAddress = ipAddress
        self.lastStatusTime = lastStatusTime
        self.portName = portName
        self.gitSha = gitSha
        self.buildTimeUs = buildTimeUs
        self.owdUs = owdUs
        self.qos = qos
    }

    var id: Int { clientId }

    // Controller time-since-boot from the QoS block (uptime_us).
    var uptimeText: String {
        guard let us = qos?.uptimeUs else { return "—" }
        return formatDurationUs(us)
    }

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
        case portName = "port_name"
        case gitSha = "git_sha"
        case buildTimeUs = "build_time_us"
        case owdUs = "owd_us"
        case qos
    }
}
