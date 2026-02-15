import Foundation
import Combine

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
    var isLoading = false

    private var timer: AnyCancellable?
    private let api: APIClient
    private let maxHistory = 30

    init(api: APIClient) {
        self.api = api
    }

    func startPolling() {
        fetch()
        timer = Timer.publish(every: 2, on: .main, in: .common)
            .autoconnect()
            .sink { [weak self] _ in self?.fetch() }
    }

    func stopPolling() {
        timer?.cancel()
        timer = nil
    }

    func refresh() {
        fetch()
    }

    func toggleService(id: String, enabled: Bool) {
        Task { @MainActor in
            do {
                let _: ServiceControlResponse = try await api.post(
                    "/api/service/control",
                    body: ServiceControlRequest(id: id, status: enabled)
                )
                fetch()
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    private func fetch() {
        Task { @MainActor in
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
}
