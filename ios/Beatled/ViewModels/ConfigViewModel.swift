import Foundation

enum HostPreset: String, CaseIterable, Identifiable {
    case beatledLocal = "https://beatled.local:8443"
    case beatledTest = "https://beatled.test:8443"
    case raspberryPi = "https://raspberrypi1.local:8443"
    case localhost = "https://localhost:8443"
    case viteProxy = "http://localhost:5173"
    case custom = "custom"

    var id: String { rawValue }

    var label: String {
        switch self {
        case .beatledLocal: return "beatled.local"
        case .beatledTest: return "beatled.test"
        case .raspberryPi: return "Raspberry Pi"
        case .localhost: return "Localhost"
        case .viteProxy: return "Vite Proxy"
        case .custom: return "Custom"
        }
    }

    var subtitle: String? {
        guard self != .custom, let url = URL(string: rawValue) else { return nil }
        let host = url.host() ?? rawValue
        if let port = url.port {
            return "\(host):\(port)"
        }
        return host
    }

    static func from(host: String) -> HostPreset {
        HostPreset.allCases.first { $0.rawValue == host } ?? .custom
    }
}

enum HealthStatus {
    case unknown, ok, error
}

@Observable
class ConfigViewModel {
    var settings: AppSettings
    var healthStatuses: [HostPreset: HealthStatus] = [:]

    var selectedPreset: HostPreset {
        didSet {
            if selectedPreset == .custom {
                customHost = settings.apiHost
            } else {
                settings.apiHost = selectedPreset.rawValue
                customHost = ""
            }
        }
    }
    var customHost: String = "" {
        didSet {
            if selectedPreset == .custom, !customHost.isEmpty {
                settings.apiHost = customHost
            }
        }
    }

    private let healthSession: URLSession

    init(settings: AppSettings) {
        // healthSession must be initialized before any access to other stored properties
        let config = URLSessionConfiguration.ephemeral
        config.timeoutIntervalForRequest = 3
        self.healthSession = URLSession(configuration: config, delegate: InsecureTrustDelegate(), delegateQueue: nil)

        self.settings = settings
        self.selectedPreset = HostPreset.from(host: settings.apiHost)
        if self.selectedPreset == .custom {
            self.customHost = settings.apiHost
        }

        for preset in HostPreset.allCases where preset != .custom {
            healthStatuses[preset] = .unknown
        }
    }

    @MainActor
    func runHealthChecks() async {
        await checkAllHealth()
        autoSelectFirstOnline()
        while !Task.isCancelled {
            try? await Task.sleep(for: .seconds(5))
            await checkAllHealth()
        }
    }

    private func autoSelectFirstOnline() {
        guard healthStatuses[selectedPreset] != .ok else { return }
        let ordered = HostPreset.allCases.filter { $0 != .custom }
        if let first = ordered.first(where: { healthStatuses[$0] == .ok }) {
            selectedPreset = first
        }
    }

    @MainActor
    private func checkAllHealth() async {
        await withTaskGroup(of: (HostPreset, HealthStatus).self) { group in
            for preset in HostPreset.allCases where preset != .custom {
                guard let url = URL(string: preset.rawValue + "/api/health") else { continue }
                group.addTask {
                    let request = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData)
                    do {
                        let (_, response) = try await self.healthSession.data(for: request)
                        if let http = response as? HTTPURLResponse, (200...299).contains(http.statusCode) {
                            return (preset, .ok)
                        }
                        return (preset, .error)
                    } catch {
                        return (preset, .error)
                    }
                }
            }
            for await (preset, status) in group {
                healthStatuses[preset] = status
            }
        }
    }
}

/// Trusts all server certificates so health checks work with self-signed certs.
private class InsecureTrustDelegate: NSObject, URLSessionDelegate {
    func urlSession(
        _ session: URLSession,
        didReceive challenge: URLAuthenticationChallenge,
        completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void
    ) {
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
           let trust = challenge.protectionSpace.serverTrust {
            completionHandler(.useCredential, URLCredential(trust: trust))
        } else {
            completionHandler(.performDefaultHandling, nil)
        }
    }
}
