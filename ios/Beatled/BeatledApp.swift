import SwiftUI

@main
struct BeatledApp: App {
    @State private var settings = AppSettings()
    @State private var apiClient: APIClient?

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(settings)
                .environment(apiClient ?? createClient())
                .tint(Color("AccentColor"))
        }
        #if os(macOS)
        .defaultSize(width: 900, height: 600)
        .windowToolbarStyle(.unified)
        #endif
    }

    private func createClient() -> APIClient {
        let client = APIClient(settings: settings)
        Task { @MainActor in apiClient = client }
        return client
    }
}
