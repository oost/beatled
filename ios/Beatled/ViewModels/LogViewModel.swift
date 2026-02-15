import Foundation

@Observable
class LogViewModel {
    var lines: [String] = []
    var error: String?

    private var pollingTask: Task<Void, Never>?
    private let api: APIClient

    init(api: APIClient) {
        self.api = api
    }

    func startPolling() {
        pollingTask?.cancel()
        pollingTask = Task { @MainActor in
            while !Task.isCancelled {
                await fetch()
                try? await Task.sleep(for: .seconds(10))
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

    @MainActor
    private func fetch() async {
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
