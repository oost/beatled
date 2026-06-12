import XCTest
@testable import Beatled

/// Pure logic from the Config screen: preset matching, display strings, and
/// the host-write-through rules in ConfigViewModel's didSet observers.
final class ConfigViewModelTests: XCTestCase {
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

    private func makeSettings(host: String? = nil) -> AppSettings {
        if let host {
            defaults.set(host, forKey: "apiHost")
        }
        return AppSettings(tokenStore: InMemoryTokenStore(), defaults: defaults)
    }

    // MARK: HostPreset

    func testHostPresetFromKnownHost() {
        XCTAssertEqual(HostPreset.from(host: "https://beatled.local:8443"), .beatledLocal)
        XCTAssertEqual(HostPreset.from(host: "https://localhost:8443"), .localhost)
        XCTAssertEqual(HostPreset.from(host: "http://localhost:5173"), .viteProxy)
    }

    func testHostPresetFromUnknownHostIsCustom() {
        XCTAssertEqual(HostPreset.from(host: "https://example.com:9999"), .custom)
        XCTAssertEqual(HostPreset.from(host: ""), .custom)
    }

    func testHostPresetSubtitleShowsHostAndPort() {
        XCTAssertEqual(HostPreset.beatledLocal.subtitle, "beatled.local:8443")
        XCTAssertEqual(HostPreset.viteProxy.subtitle, "localhost:5173")
        XCTAssertNil(HostPreset.custom.subtitle)
    }

    // MARK: ConfigViewModel

    func testInitSelectsMatchingPreset() {
        let viewModel = ConfigViewModel(settings: makeSettings(host: "https://beatled.local:8443"),
                                        api: APIClient(settings: makeSettings()))
        XCTAssertEqual(viewModel.selectedPreset, .beatledLocal)
        XCTAssertEqual(viewModel.customHost, "")
    }

    func testInitWithUnknownHostSelectsCustomAndKeepsHost() {
        let viewModel = ConfigViewModel(settings: makeSettings(host: "https://example.com:9999"),
                                        api: APIClient(settings: makeSettings()))
        XCTAssertEqual(viewModel.selectedPreset, .custom)
        XCTAssertEqual(viewModel.customHost, "https://example.com:9999")
    }

    func testSelectingPresetWritesHostToSettings() {
        let settings = makeSettings()
        let viewModel = ConfigViewModel(settings: settings, api: APIClient(settings: settings))

        viewModel.selectedPreset = .raspberryPi

        XCTAssertEqual(settings.apiHost, HostPreset.raspberryPi.rawValue)
        XCTAssertEqual(viewModel.customHost, "")
    }

    func testCustomHostWritesThroughOnlyWhenCustomSelected() {
        let settings = makeSettings(host: "https://example.com:9999")
        let viewModel = ConfigViewModel(settings: settings, api: APIClient(settings: settings))

        viewModel.customHost = "https://other.example:1234"
        XCTAssertEqual(settings.apiHost, "https://other.example:1234")

        viewModel.selectedPreset = .localhost
        viewModel.customHost = "https://ignored.example:1"
        XCTAssertEqual(settings.apiHost, HostPreset.localhost.rawValue,
                       "custom host edits must not apply while a preset is selected")
    }
}
