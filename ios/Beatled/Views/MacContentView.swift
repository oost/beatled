#if os(macOS)
import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable {
    // Program first, matching the web client's default route.
    case program, status, log, settings
    var id: Self { self }

    var label: String { rawValue.capitalized }

    var icon: String {
        switch self {
        case .program:  "music.note"
        case .status:   "waveform.path"
        case .log:      "text.alignleft"
        case .settings: "gearshape"
        }
    }
}

struct MacContentView: View {
    let api: APIClient
    let configViewModel: ConfigViewModel
    @State private var selection: SidebarItem? = .program

    var body: some View {
        NavigationSplitView {
            List(SidebarItem.allCases, selection: $selection) { item in
                Label(item.label, systemImage: item.icon)
            }
            .navigationSplitViewColumnWidth(min: 160, ideal: 200)
        } detail: {
            switch selection {
            case .program:
                ProgramView(
                    viewModel: ProgramViewModel(api: api),
                    statusViewModel: StatusViewModel(api: api)
                )
            case .status:   StatusView(viewModel: StatusViewModel(api: api))
            case .log:      LogView(viewModel: LogViewModel(api: api))
            case .settings: ConfigView(viewModel: configViewModel)
            case nil:       Text("Select a view")
            }
        }
    }
}
#endif
