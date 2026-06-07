import Foundation

@Observable
class AppSettings {
    /// UserDefaults key that previously held the API token in plaintext.
    /// We migrate it into the Keychain on first launch and clear it.
    private static let legacyTokenKey = "apiToken"

    var apiHost: String {
        didSet { UserDefaults.standard.set(apiHost, forKey: "apiHost") }
    }
    /// Bearer API token. Persisted in the Keychain via KeychainTokenStore;
    /// the in-memory copy is kept so SwiftUI bindings (e.g. the SecureField
    /// on the Config screen) drive observed updates without round-tripping
    /// the keychain on every keystroke.
    var apiToken: String {
        didSet { KeychainTokenStore.save(apiToken) }
    }
    var allowInsecureConnections: Bool {
        didSet { UserDefaults.standard.set(allowInsecureConnections, forKey: "allowInsecureConnections") }
    }

    init() {
        self.apiHost = UserDefaults.standard.string(forKey: "apiHost") ?? "https://localhost:8443"

        // One-shot migration: if a legacy plaintext token still lives in
        // UserDefaults, move it into the Keychain and clear the old key. We
        // do this before reading the canonical value so a user upgrading
        // from a pre-Keychain build sees their token intact.
        let defaults = UserDefaults.standard
        if let legacy = defaults.string(forKey: Self.legacyTokenKey), !legacy.isEmpty {
            // Only migrate if the Keychain doesn't already have something
            // (don't clobber a freshly-set value from a parallel device).
            if KeychainTokenStore.load() == nil {
                KeychainTokenStore.save(legacy)
            }
            defaults.removeObject(forKey: Self.legacyTokenKey)
        }

        self.apiToken = KeychainTokenStore.load() ?? ""
        self.allowInsecureConnections = UserDefaults.standard.bool(forKey: "allowInsecureConnections")
    }
}
