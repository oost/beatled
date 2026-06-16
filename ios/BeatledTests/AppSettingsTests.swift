import XCTest
@testable import Beatled

/// In-memory TokenStore fake mirroring KeychainTokenStore's semantics
/// (empty string deletes) so AppSettings' migration and persistence logic
/// can run without the real Keychain.
final class InMemoryTokenStore: TokenStore {
    private(set) var token: String?

    func load() -> String? { token }

    @discardableResult
    func save(_ token: String) -> Bool {
        if token.isEmpty { return delete() }
        self.token = token
        return true
    }

    @discardableResult
    func delete() -> Bool {
        token = nil
        return true
    }
}

final class AppSettingsTests: XCTestCase {
    private var defaults: UserDefaults!
    private var suiteName: String!

    override func setUp() {
        super.setUp()
        suiteName = "BeatledTests.\(UUID().uuidString)"
        defaults = UserDefaults(suiteName: suiteName)
    }

    override func tearDown() {
        defaults.removePersistentDomain(forName: suiteName)
        defaults = nil
        super.tearDown()
    }

    // MARK: Legacy token migration

    func testMigratesLegacyTokenFromUserDefaultsIntoStore() {
        defaults.set("legacy-token", forKey: "apiToken")
        let store = InMemoryTokenStore()

        let settings = AppSettings(tokenStore: store, defaults: defaults)

        XCTAssertEqual(store.token, "legacy-token")
        XCTAssertEqual(settings.apiToken, "legacy-token")
        XCTAssertNil(defaults.string(forKey: "apiToken"),
                     "legacy plaintext token must be cleared from UserDefaults")
    }

    func testMigrationDoesNotClobberExistingStoreToken() {
        defaults.set("legacy-token", forKey: "apiToken")
        let store = InMemoryTokenStore()
        store.save("keychain-token")

        let settings = AppSettings(tokenStore: store, defaults: defaults)

        XCTAssertEqual(store.token, "keychain-token")
        XCTAssertEqual(settings.apiToken, "keychain-token")
        XCTAssertNil(defaults.string(forKey: "apiToken"),
                     "legacy key is cleared even when the store wins")
    }

    func testEmptyLegacyTokenIsNotMigrated() {
        defaults.set("", forKey: "apiToken")
        let store = InMemoryTokenStore()

        let settings = AppSettings(tokenStore: store, defaults: defaults)

        XCTAssertNil(store.token)
        XCTAssertEqual(settings.apiToken, "")
    }

    // MARK: Token persistence

    func testApiTokenFallsBackToEmptyStringWhenStoreIsEmpty() {
        let settings = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(settings.apiToken, "")
    }

    func testSettingApiTokenWritesThroughToStore() {
        let store = InMemoryTokenStore()
        let settings = AppSettings(tokenStore: store, defaults: defaults)

        settings.apiToken = "new-token"

        XCTAssertEqual(store.token, "new-token")
    }

    func testClearingApiTokenDeletesFromStore() {
        let store = InMemoryTokenStore()
        store.save("old-token")
        let settings = AppSettings(tokenStore: store, defaults: defaults)

        settings.apiToken = ""

        XCTAssertNil(store.token)
    }

    // MARK: Host / insecure-connections persistence

    func testApiHostDefaultsToLocalhostAndPersists() {
        let settings = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(settings.apiHost, "https://localhost:8443")

        settings.apiHost = "https://beatled.local:8443"
        XCTAssertEqual(defaults.string(forKey: "apiHost"), "https://beatled.local:8443")

        let reloaded = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(reloaded.apiHost, "https://beatled.local:8443")
    }

    func testAllowInsecureConnectionsDefaultsToFalseAndPersists() {
        let settings = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertFalse(settings.allowInsecureConnections)

        settings.allowInsecureConnections = true
        XCTAssertTrue(defaults.bool(forKey: "allowInsecureConnections"))

        let reloaded = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertTrue(reloaded.allowInsecureConnections)
    }

    // MARK: Appearance persistence

    func testAppearanceDefaultsToSystemAndPersists() {
        let settings = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(settings.appearance, .system)

        settings.appearance = .dark
        XCTAssertEqual(defaults.string(forKey: "appearance"), "dark")

        let reloaded = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(reloaded.appearance, .dark)
    }

    func testUnknownStoredAppearanceFallsBackToSystem() {
        defaults.set("chartreuse", forKey: "appearance")
        let settings = AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
        XCTAssertEqual(settings.appearance, .system)
    }
}
