#if os(macOS)
import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable {
    case status, program, log, config
    var id: Self { self }

    var label: String { rawValue.capitalized }

    var icon: String {
        switch self {
        case .status:  "waveform.path"
        case .program: "music.note"
        case .log:     "text.alignleft"
        case .config:  "gearshape"
        }
    }
}

struct MacContentView: View {
    let api: APIClient
    let configViewModel: ConfigViewModel
    @State private var selection: SidebarItem? = .status

    var body: some View {
        NavigationSplitView {
            List(SidebarItem.allCases, selection: $selection) { item in
                Label(item.label, systemImage: item.icon)
            }
            .navigationSplitViewColumnWidth(min: 160, ideal: 200)
        } detail: {
            switch selection {
            case .status:  StatusView(viewModel: StatusViewModel(api: api))
            case .program: ProgramView(viewModel: ProgramViewModel(api: api))
            case .log:     LogView(viewModel: LogViewModel(api: api))
            case .config:  ConfigView(viewModel: configViewModel)
            case nil:      Text("Select a view")
            }
        }
    }
}
#endif
