import Foundation
import Security

/// Abstracts token persistence so the migration / fallback logic in
/// `AppSettings` can be unit-tested with an in-memory fake. The only real
/// implementation in the app is `KeychainTokenStore`.
protocol TokenStore {
    /// Read the token, or nil if none is set (or the store is unavailable).
    func load() -> String?

    /// Save (or replace) the token. An empty string deletes instead — that
    /// matches the legacy UserDefaults behaviour where an empty string meant
    /// "no token configured".
    @discardableResult
    func save(_ token: String) -> Bool

    @discardableResult
    func delete() -> Bool
}

/// Persists the bearer API token in the iOS / macOS Keychain.
///
/// The Beatled API token authorises every read and write on the server
/// (program changes, service control, status). Storing it in UserDefaults
/// was a Tier-1 finding: UserDefaults lives in the app sandbox's preferences
/// plist, which is included in unencrypted iTunes / iCloud device backups and
/// is readable by any process that mounts the simulator's container.
///
/// This wrapper keeps the wire-compatible API minimal — `save`, `load`,
/// `delete` over a single (service, account) pair — so callers can use it
/// like a typed `String?` accessor.
struct KeychainTokenStore: TokenStore {
    /// Bundle-suffixed service tag so multiple builds of the app on the same
    /// device (debug vs. release vs. TestFlight) don't share keychain items.
    static let defaultService: String = {
        let base = "com.beatled.api-token"
        if let bundle = Bundle.main.bundleIdentifier {
            return "\(base).\(bundle)"
        }
        return base
    }()

    private let service: String

    /// Single-user app — one account row is enough. If we ever multi-tenant
    /// the app, switch this to a user-id or host hash.
    private let account: String

    /// The non-default service/account are for tests, which use a throwaway
    /// service tag so they never touch the user's real token.
    init(service: String = KeychainTokenStore.defaultService, account: String = "default") {
        self.service = service
        self.account = account
    }

    /// Read the token, or nil if none is set. Returns nil on any keychain
    /// failure rather than throwing — callers treat absence and read errors
    /// identically (the network call will just fail with 401).
    func load() -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]

        var item: CFTypeRef?
        let status = SecItemCopyMatching(query as CFDictionary, &item)
        guard status == errSecSuccess,
              let data = item as? Data,
              let token = String(data: data, encoding: .utf8) else {
            return nil
        }
        return token
    }

    /// Save (or replace) the token. An empty string deletes instead — that
    /// matches the legacy UserDefaults behaviour where an empty string meant
    /// "no token configured".
    @discardableResult
    func save(_ token: String) -> Bool {
        if token.isEmpty {
            return delete()
        }
        guard let data = token.data(using: .utf8) else { return false }

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account
        ]
        let attributes: [String: Any] = [
            kSecValueData as String: data,
            kSecAttrAccessible as String: kSecAttrAccessibleAfterFirstUnlock
        ]

        // Update first; if no item exists, fall through to add.
        let updateStatus = SecItemUpdate(query as CFDictionary, attributes as CFDictionary)
        if updateStatus == errSecSuccess {
            return true
        }
        if updateStatus != errSecItemNotFound {
            return false
        }

        var addQuery = query
        addQuery.merge(attributes) { _, new in new }
        let addStatus = SecItemAdd(addQuery as CFDictionary, nil)
        return addStatus == errSecSuccess
    }

    @discardableResult
    func delete() -> Bool {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account
        ]
        let status = SecItemDelete(query as CFDictionary)
        return status == errSecSuccess || status == errSecItemNotFound
    }
}
