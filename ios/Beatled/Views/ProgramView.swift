import SwiftUI

struct ProgramView: View {
    @Bindable var viewModel: ProgramViewModel

    var body: some View {
        NavigationStack {
            List {
                if let error = viewModel.error {
                    Section {
                        Label(error, systemImage: "exclamationmark.triangle")
                            .foregroundStyle(.red)
                            .font(.caption)
                    }
                }

                programsSection
            }
            .navigationTitle("Program")
            .overlay {
                if viewModel.isLoading && viewModel.programs.isEmpty {
                    ProgressView()
                } else if !viewModel.isLoading && viewModel.programs.isEmpty && viewModel.error == nil {
                    ContentUnavailableView {
                        Label("No Programs", systemImage: "music.note.list")
                    } description: {
                        Text("No programs available on the server.")
                    }
                }
            }
            .onAppear { viewModel.load() }
            .refreshable { viewModel.load() }
        }
    }

    @ViewBuilder
    private var programsSection: some View {
        if !viewModel.programs.isEmpty {
            Section("Programs") {
                Picker(selection: Binding(
                    get: { viewModel.selectedProgramId },
                    set: { viewModel.selectProgram($0) }
                )) {
                    ForEach(viewModel.programs) { program in
                        Text(program.name)
                            .tag(program.id)
                    }
                } label: {
                    EmptyView()
                }
                .pickerStyle(.inline)
                .labelsHidden()
            }
        }
    }
}
