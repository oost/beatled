import SwiftUI

struct LogView: View {
    @Bindable var viewModel: LogViewModel

    var body: some View {
        NavigationStack {
            ScrollViewReader { proxy in
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 1) {
                        ForEach(Array(viewModel.lines.enumerated()), id: \.offset) { index, line in
                            Text(line)
                                .font(.system(.caption, design: .monospaced))
                                .textSelection(.enabled)
                                .id(index)
                        }
                    }
                    .padding(.horizontal)
                }
                .onChange(of: viewModel.lines.count) {
                    if let last = viewModel.lines.indices.last {
                        withAnimation {
                            proxy.scrollTo(last, anchor: .bottom)
                        }
                    }
                }
            }
            .navigationTitle("Log")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button {
                        viewModel.refresh()
                    } label: {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .overlay {
                if let error = viewModel.error, viewModel.lines.isEmpty {
                    ContentUnavailableView(
                        "Cannot Load Logs",
                        systemImage: "exclamationmark.triangle",
                        description: Text(error)
                    )
                }
            }
            .onAppear { viewModel.startPolling() }
            .onDisappear { viewModel.stopPolling() }
        }
    }
}
