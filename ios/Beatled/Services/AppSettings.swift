import Foundation

@Observable
class AppSettings {
    var apiHost: String {
        didSet { UserDefaults.standard.set(apiHost, forKey: "apiHost") }
    }
    var apiToken: String {
        didSet { UserDefaults.standard.set(apiToken, forKey: "apiToken") }
    }
    var allowInsecureConnections: Bool {
        didSet { UserDefaults.standard.set(allowInsecureConnections, forKey: "allowInsecureConnections") }
    }

    init() {
        self.apiHost = UserDefaults.standard.string(forKey: "apiHost") ?? "https://localhost:8443"
        self.apiToken = UserDefaults.standard.string(forKey: "apiToken") ?? ""
        self.allowInsecureConnections = UserDefaults.standard.bool(forKey: "allowInsecureConnections")
    }
}
