import Foundation
import Combine

@Observable
class LogViewModel {
    var lines: [String] = []
    var error: String?
    var isLoading = false

    private var timer: AnyCancellable?
    private let api: APIClient

    init(api: APIClient) {
        self.api = api
    }

    func startPolling() {
        fetch()
        timer = Timer.publish(every: 10, on: .main, in: .common)
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

    private func fetch() {
        Task { @MainActor in
            do {
                let data = try await api.getRaw("/api/log")
                let decoded = try JSONDecoder().decode([String].self, from: data)
                self.lines = decoded
                self.error = nil
            } catch {
                self.error = error.localizedDescription
            }
        }
    }
}
