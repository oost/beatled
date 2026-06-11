import XCTest
@testable import Beatled

/// Exercises the real Keychain implementation. Each test uses a throwaway
/// service tag so it never touches the user's actual API token, and skips
/// (rather than fails) when the environment has no usable Keychain — e.g. an
/// unsigned CI runner with a locked login keychain.
final class KeychainTokenStoreTests: XCTestCase {
    /// A store on a unique service, pre-flighted for Keychain availability.
    /// Registers a teardown so no test leaves items behind.
    private func makeStore() throws -> KeychainTokenStore {
        let store = KeychainTokenStore(service: "com.beatled.tests.\(UUID().uuidString)")
        guard store.save("availability-probe") else {
            throw XCTSkip("Keychain is not available in this environment")
        }
        addTeardownBlock { store.delete() }
        store.delete()
        return store
    }

    func testLoadReturnsNilWhenNothingSaved() throws {
        let store = try makeStore()
        XCTAssertNil(store.load())
    }

    func testSaveThenLoadRoundTrips() throws {
        let store = try makeStore()
        XCTAssertTrue(store.save("secret-token"))
        XCTAssertEqual(store.load(), "secret-token")
    }

    func testSaveOverwritesExistingToken() throws {
        let store = try makeStore()
        store.save("first")
        XCTAssertTrue(store.save("second"))
        XCTAssertEqual(store.load(), "second")
    }

    func testSaveEmptyStringDeletes() throws {
        let store = try makeStore()
        store.save("secret-token")
        XCTAssertTrue(store.save(""))
        XCTAssertNil(store.load())
    }

    func testDeleteRemovesToken() throws {
        let store = try makeStore()
        store.save("secret-token")
        XCTAssertTrue(store.delete())
        XCTAssertNil(store.load())
    }

    func testDeleteIsIdempotent() throws {
        let store = try makeStore()
        XCTAssertTrue(store.delete(), "deleting a missing item should still succeed")
    }

    func testStoresWithDifferentServicesAreIsolated() throws {
        let store = try makeStore()
        let other = try makeStore()
        store.save("mine")
        XCTAssertNil(other.load())
    }
}
