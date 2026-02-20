import SwiftUI

@main
struct BeatledApp: App {
    @State private var settings: AppSettings
    @State private var apiClient: APIClient
    @State private var configViewModel: ConfigViewModel

    init() {
        let settings = AppSettings()
        let client = APIClient(settings: settings)
        self._settings = State(initialValue: settings)
        self._apiClient = State(initialValue: client)
        self._configViewModel = State(initialValue: ConfigViewModel(settings: settings))
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(settings)
                .environment(apiClient)
                .environment(configViewModel)
                .tint(Color("AccentColor"))
                .task { await configViewModel.runHealthChecks() }
        }
        #if os(macOS)
        .defaultSize(width: 900, height: 600)
        .windowToolbarStyle(.unified)
        #endif
    }
}
