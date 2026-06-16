import Foundation
import SwiftUI

/// User's preferred appearance, mirroring the web client's System/Light/Dark
/// control so all three clients expose the same choice.
enum AppearancePreference: String, CaseIterable, Identifiable {
    case system, light, dark

    var id: String { rawValue }

    var label: String {
        switch self {
        case .system: "System"
        case .light: "Light"
        case .dark: "Dark"
        }
    }

    /// nil follows the OS appearance; otherwise force the scheme via
    /// `.preferredColorScheme`.
    var colorScheme: ColorScheme? {
        switch self {
        case .system: nil
        case .light: .light
        case .dark: .dark
        }
    }
}

@Observable
class AppSettings {
    /// UserDefaults key that previously held the API token in plaintext.
    /// We migrate it into the Keychain on first launch and clear it.
    private static let legacyTokenKey = "apiToken"

    /// Both stores are injectable so the migration logic below is testable
    /// with an in-memory TokenStore and a throwaway UserDefaults suite.
    @ObservationIgnored private let tokenStore: TokenStore
    @ObservationIgnored private let defaults: UserDefaults

    var apiHost: String {
        didSet { defaults.set(apiHost, forKey: "apiHost") }
    }
    /// Bearer API token. Persisted in the Keychain via KeychainTokenStore;
    /// the in-memory copy is kept so SwiftUI bindings (e.g. the SecureField
    /// on the Config screen) drive observed updates without round-tripping
    /// the keychain on every keystroke.
    var apiToken: String {
        didSet { tokenStore.save(apiToken) }
    }
    var allowInsecureConnections: Bool {
        didSet { defaults.set(allowInsecureConnections, forKey: "allowInsecureConnections") }
    }
    var appearance: AppearancePreference {
        didSet { defaults.set(appearance.rawValue, forKey: "appearance") }
    }

    init(tokenStore: TokenStore = KeychainTokenStore(), defaults: UserDefaults = .standard) {
        self.tokenStore = tokenStore
        self.defaults = defaults
        self.apiHost = defaults.string(forKey: "apiHost") ?? "https://localhost:8443"

        // One-shot migration: if a legacy plaintext token still lives in
        // UserDefaults, move it into the Keychain and clear the old key. We
        // do this before reading the canonical value so a user upgrading
        // from a pre-Keychain build sees their token intact.
        if let legacy = defaults.string(forKey: Self.legacyTokenKey), !legacy.isEmpty {
            // Only migrate if the Keychain doesn't already have something
            // (don't clobber a freshly-set value from a parallel device).
            if tokenStore.load() == nil {
                tokenStore.save(legacy)
            }
            defaults.removeObject(forKey: Self.legacyTokenKey)
        }

        self.apiToken = tokenStore.load() ?? ""
        self.allowInsecureConnections = defaults.bool(forKey: "allowInsecureConnections")
        self.appearance = AppearancePreference(
            rawValue: defaults.string(forKey: "appearance") ?? "") ?? .system
    }
}
