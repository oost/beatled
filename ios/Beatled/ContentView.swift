import SwiftUI

struct ContentView: View {
    @Environment(APIClient.self) private var api
    @Environment(ConfigViewModel.self) private var configViewModel

    var body: some View {
        #if os(iOS)
        TabView {
            // Program first, matching the web client's default route.
            Tab("Program", systemImage: "music.note") {
                ProgramView(
                    viewModel: ProgramViewModel(api: api),
                    statusViewModel: StatusViewModel(api: api)
                )
            }
            Tab("Status", systemImage: "waveform.path") {
                StatusView(viewModel: StatusViewModel(api: api))
            }
            Tab("Log", systemImage: "text.alignleft") {
                LogView(viewModel: LogViewModel(api: api))
            }
            Tab("Settings", systemImage: "gearshape") {
                ConfigView(viewModel: configViewModel)
            }
        }
        .tabBarMinimizeBehavior(.onScrollDown)
        #else
        MacContentView(api: api, configViewModel: configViewModel)
        #endif
    }
}
