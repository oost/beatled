import Foundation

struct TempoReading: Identifiable {
    let id = UUID()
    let time: Date
    let bpm: Double
}

@Observable
class StatusViewModel {
    var status: StatusResponse?
    var devices: [Device] = []
    var tempoHistory: [TempoReading] = []
    var lastUpdate: Date?
    var error: String?

    private var pollingTask: Task<Void, Never>?
    private let api: APIClient
    private let maxHistory = 30

    init(api: APIClient) {
        self.api = api
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

    func toggleService(id: String, enabled: Bool) {
        Task { @MainActor in
            do {
                let _: ServiceControlResponse = try await api.post(
                    "/api/service/control",
                    body: ServiceControlRequest(id: id, status: enabled)
                )
                await fetch()
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    @MainActor
    private func fetch() async {
        do {
            async let statusReq: StatusResponse = api.get("/api/status")
            async let devicesReq: DevicesResponse = api.get("/api/devices")
            let (s, d) = try await (statusReq, devicesReq)
            self.status = s
            self.devices = d.devices
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
