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

    // Access-point (hotspot) control via POST /api/ap.
    var apStatus: String = "unknown" // "on", "off", or "unknown"
    var apMessage: String?
    var apBusy = false
    var revertMinutes: Int = 10

    private let api: APIClient

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

    init(settings: AppSettings, api: APIClient) {
        self.settings = settings
        self.api = api
        self.selectedPreset = HostPreset.from(host: settings.apiHost)
        if self.selectedPreset == .custom {
            self.customHost = settings.apiHost
        }

        for preset in HostPreset.allCases where preset != .custom {
            healthStatuses[preset] = .unknown
        }
    }

    /// Build a URLSession for the discovery health checks. When the user has
    /// `allowInsecureConnections` enabled we attach a delegate that accepts
    /// any server certificate (including the mkcert self-signed bundle used
    /// by the local dev server); otherwise we use the platform's default
    /// validation so production hosts are properly verified.
    ///
    /// The session is ephemeral and cheap to construct, so we rebuild on
    /// every batch rather than cache it. That also means a setting toggle
    /// takes effect immediately on the next 5-second poll, no view restart.
    private func makeHealthSession() -> URLSession {
        let config = URLSessionConfiguration.ephemeral
        config.timeoutIntervalForRequest = 3
        if settings.allowInsecureConnections {
            return URLSession(configuration: config,
                              delegate: InsecureTrustDelegate(),
                              delegateQueue: nil)
        }
        return URLSession(configuration: config)
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
        let session = makeHealthSession()
        await withTaskGroup(of: (HostPreset, HealthStatus).self) { group in
            for preset in HostPreset.allCases where preset != .custom {
                guard let url = URL(string: preset.rawValue + "/api/health") else { continue }
                group.addTask {
                    let request = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData)
                    do {
                        let (_, response) = try await session.data(for: request)
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

    // MARK: - Access point control

    @MainActor
    func refreshAPStatus() async {
        do {
            let resp: APStatusResponse = try await api.post(
                "/api/ap", body: APControlRequest(mode: "status", revertMinutes: nil))
            apStatus = resp.ap
        } catch {
            apStatus = "unknown"
        }
    }

    /// Switch the Pi to hotspot mode. The single radio means this tears down
    /// the link this request travels over, so the response usually never
    /// arrives — we don't depend on it and leave the guidance message up.
    @MainActor
    func switchAPOn() {
        apBusy = true
        apMessage = "Switching to hotspot — this device will lose connection. "
            + "Rejoin the \"Beatled\" WiFi and open https://192.168.4.1:8443/."
        Task { @MainActor in
            let mins = revertMinutes > 0 ? revertMinutes : nil
            let _: APActionResponse? = try? await api.post(
                "/api/ap", body: APControlRequest(mode: "on", revertMinutes: mins))
            apBusy = false
        }
    }

    @MainActor
    func switchAPOff() {
        apBusy = true
        apMessage = nil
        Task { @MainActor in
            do {
                let _: APActionResponse = try await api.post(
                    "/api/ap", body: APControlRequest(mode: "off", revertMinutes: nil))
                await refreshAPStatus()
            } catch {
                apMessage = "Failed to switch to WiFi: \(error.localizedDescription)"
            }
            apBusy = false
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
