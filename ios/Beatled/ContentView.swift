import SwiftUI

struct ContentView: View {
    @Environment(APIClient.self) private var api
    @Environment(ConfigViewModel.self) private var configViewModel

    var body: some View {
        #if os(iOS)
        TabView {
            Tab("Status", systemImage: "waveform.path") {
                StatusView(viewModel: StatusViewModel(api: api))
            }
            Tab("Program", systemImage: "music.note") {
                ProgramView(viewModel: ProgramViewModel(api: api))
            }
            Tab("Log", systemImage: "text.alignleft") {
                LogView(viewModel: LogViewModel(api: api))
            }
            Tab("Config", systemImage: "gearshape") {
                ConfigView(viewModel: configViewModel)
            }
        }
        .tabBarMinimizeBehavior(.onScrollDown)
        #else
        MacContentView(api: api, configViewModel: configViewModel)
        #endif
    }
}
