import Foundation

struct TempoReading: Identifiable {
    let id = UUID()
    let time: Date
    let bpm: Double
}

// The two tempo sources the UI exposes. They map to the mutually-exclusive
// `beat-detector` and `manual-bpm` services on the server.
enum TempoMode: Hashable {
    case beat
    case manual
}

@Observable
class StatusViewModel {
    static let beatDetectorId = "beat-detector"
    static let manualBpmId = "manual-bpm"

    var status: StatusResponse?
    var devices: [Device] = []
    var qos: FleetQos?
    var tempoHistory: [TempoReading] = []
    var lastUpdate: Date?
    var error: String?

    private var pollingTask: Task<Void, Never>?
    private let api: APIClient
    // Must comfortably cover the chart's sliding 60s window at one reading
    // per 2s poll, so the line reaches the left edge.
    private let maxHistory = 90

    init(api: APIClient) {
        self.api = api
    }

    // Strict two-mode model matching the web client: one tempo source is
    // always presented as active. If the server reports both services off
    // (e.g. right after boot) we render beat tracking without posting
    // anything until the user interacts.
    var tempoMode: TempoMode {
        (status?.status[Self.manualBpmId] ?? false) ? .manual : .beat
    }

    func startPolling() {
        pollingTask?.cancel()
        pollingTask = Task { @MainActor in
            while !Task.isCancelled {
                await fetch()
                try? await Task.sleep(for: .seconds(2))
            }
        }
    }

    func stopPolling() {
        pollingTask?.cancel()
        pollingTask = nil
    }

    func refresh() {
        Task { @MainActor in await fetch() }
    }

    func setTempoMode(_ mode: TempoMode) {
        Task { @MainActor in
            do {
                let target = mode == .manual ? Self.manualBpmId : Self.beatDetectorId
                let other = mode == .manual ? Self.beatDetectorId : Self.manualBpmId
                // The server stops the other tempo source when one starts;
                // the explicit second call keeps the strict two-mode
                // contract client-side too.
                try await serviceControl(id: target, enabled: true)
                try await serviceControl(id: other, enabled: false)
                await fetch()
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    func setManualBpm(_ bpm: Double) {
        Task { @MainActor in
            do {
                struct Body: Encodable { let bpm: Double }
                struct Response: Decodable { let manualBpm: Double }
                let _: Response = try await api.post("/api/tempo/manual", body: Body(bpm: bpm))
                await fetch()
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    func toggleService(id: String, enabled: Bool) {
        Task { @MainActor in
            do {
                try await serviceControl(id: id, enabled: enabled)
                await fetch()
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    private func serviceControl(id: String, enabled: Bool) async throws {
        let _: ServiceControlResponse = try await api.post(
            "/api/service/control",
            body: ServiceControlRequest(id: id, status: enabled)
        )
    }

    @MainActor
    private func fetch() async {
        do {
            async let statusReq: StatusResponse = api.get("/api/status")
            async let devicesReq: DevicesResponse = api.get("/api/devices")
            let (s, d) = try await (statusReq, devicesReq)
            self.status = s
            self.devices = d.devices
            // Soft-fail like the web client: older servers without /api/qos
            // (or a momentary failure) just leave the card empty.
            let q: FleetQos? = try? await api.get("/api/qos")
            self.qos = q
            self.lastUpdate = Date()
            self.error = nil

            let reading = TempoReading(time: Date(), bpm: s.tempo)
            tempoHistory.append(reading)
            if tempoHistory.count > maxHistory {
                tempoHistory.removeFirst(tempoHistory.count - maxHistory)
            }
        } catch {
            self.error = error.localizedDescription
        }
    }
}
