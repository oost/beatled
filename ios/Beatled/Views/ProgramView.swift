import SwiftUI

struct ProgramView: View {
    @Bindable var viewModel: ProgramViewModel

    var body: some View {
        NavigationStack {
            List {
                errorSection
                programsSection
            }
            .navigationTitle("Program")
            .overlay {
                loadingOverlay
            }
            .onAppear { viewModel.load() }
            .refreshable { viewModel.load() }
        }
    }
    
    @ViewBuilder
    private var errorSection: some View {
        if let error = viewModel.error {
            Section {
                Label(error, systemImage: "exclamationmark.triangle")
                    .foregroundStyle(.red)
                    .font(.caption)
            }
        }
    }
    
    @ViewBuilder
    private var programsSection: some View {
        Section("Programs") {
            ForEach(viewModel.programs, id: \.id) { program in
                programRow(program)
            }
        }
    }
    
    private func programRow(_ program: Program) -> some View {
        Button {
            viewModel.selectProgram(program.id)
        } label: {
            HStack {
                Text(program.name)
                    .foregroundStyle(.primary)
                Spacer()
                if program.id == viewModel.selectedProgramId {
                    Image(systemName: "checkmark")
                        .foregroundStyle(Color.accentColor)
                        .fontWeight(.semibold)
                }
            }
        }
    }
    
    @ViewBuilder
    private var loadingOverlay: some View {
        if viewModel.isLoading && viewModel.programs.isEmpty {
            ProgressView()
        }
    }
}
