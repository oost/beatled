import SwiftUI

struct ContentView: View {
    @Environment(APIClient.self) private var api
    @Environment(AppSettings.self) private var settings

    var body: some View {
        TabView {
            StatusView(viewModel: StatusViewModel(api: api))
                .tabItem {
                    Label("Status", systemImage: "waveform.path")
                }

            ProgramView(viewModel: ProgramViewModel(api: api))
                .tabItem {
                    Label("Program", systemImage: "music.note")
                }

            LogView(viewModel: LogViewModel(api: api))
                .tabItem {
                    Label("Log", systemImage: "text.alignleft")
                }

            ConfigView(viewModel: ConfigViewModel(settings: settings))
                .tabItem {
                    Label("Config", systemImage: "gearshape")
                }
        }
    }
}
