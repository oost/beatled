import SwiftUI

// Mirrors the web client's Program page: a grid of selectable program tiles,
// a strict two-mode tempo selector (beat tracking / manual) with a BPM
// slider that only shows in manual mode, and the tempo history chart.
struct ProgramView: View {
    @Bindable var viewModel: ProgramViewModel
    @Bindable var statusViewModel: StatusViewModel

    // Slider bounds for the manual tempo. The server accepts 20-400, but
    // practically all music sits in this range and the narrower span makes
    // individual BPM values reachable by drag (kept in step with the web
    // client).
    private static let bpmRange = 60.0...200.0

    // Local, editable copy of the manual BPM. Seeded from the server and
    // kept in sync unless the user is mid-drag, so polling doesn't yank the
    // thumb. The value is posted on release only.
    @State private var bpm: Double = 120
    @State private var sliderActive = false

    var body: some View {
        NavigationStack {
            List {
                if let error = viewModel.error ?? statusViewModel.error {
                    Section {
                        Label(error, systemImage: "exclamationmark.triangle")
                            .foregroundStyle(.red)
                            .font(.caption)
                    }
                }

                programsSection
                tempoSection
                tempoChartSection
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
            .onAppear {
                viewModel.load()
                statusViewModel.startPolling()
            }
            .onDisappear { statusViewModel.stopPolling() }
            .onChange(of: statusViewModel.status?.manualBpm) { _, newValue in
                if !sliderActive, let newValue {
                    bpm = newValue.rounded().clamped(to: Self.bpmRange)
                }
            }
            .refreshable { viewModel.load() }
        }
    }

    @ViewBuilder
    private var programsSection: some View {
        if !viewModel.programs.isEmpty {
            Section("LED Program") {
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 110), spacing: 12)], spacing: 12) {
                    ForEach(viewModel.programs) { program in
                        ProgramTile(
                            name: program.name,
                            isSelected: program.id == viewModel.selectedProgramId
                        ) {
                            viewModel.selectProgram(program.id)
                        }
                    }
                }
                .padding(.vertical, 4)
            }
        }
    }

    @ViewBuilder
    private var tempoSection: some View {
        Section("Tempo") {
            Picker("Tempo source", selection: Binding(
                get: { statusViewModel.tempoMode },
                set: { statusViewModel.setTempoMode($0) }
            )) {
                Text("Beat tracking").tag(TempoMode.beat)
                Text("Manual").tag(TempoMode.manual)
            }
            .pickerStyle(.segmented)
            .labelsHidden()

            if statusViewModel.tempoMode == .manual {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text("BPM (\(Int(Self.bpmRange.lowerBound))–\(Int(Self.bpmRange.upperBound)))")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                        Spacer()
                        Text("\(Int(bpm))")
                            .font(.callout.weight(.medium))
                            .monospacedDigit()
                    }
                    Slider(value: $bpm, in: Self.bpmRange, step: 1) { editing in
                        sliderActive = editing
                        if !editing {
                            statusViewModel.setManualBpm(bpm)
                        }
                    }
                    .accessibilityLabel("Manual BPM")
                }
            }

            HStack {
                Text("Current tempo")
                    .foregroundStyle(.secondary)
                Spacer()
                Text(currentTempoText)
                    .font(.callout.weight(.medium))
                    .monospacedDigit()
            }
        }
    }

    private var currentTempoText: String {
        if let tempo = statusViewModel.status?.tempo, tempo > 0 {
            return String(format: "%.1f BPM", tempo)
        }
        return "— BPM"
    }

    @ViewBuilder
    private var tempoChartSection: some View {
        Section("Beat History") {
            if statusViewModel.tempoHistory.isEmpty {
                ContentUnavailableView {
                    Label("No Data", systemImage: "waveform.slash")
                } description: {
                    Text("Beat detector is paused or not yet running.")
                }
                .frame(height: 160)
            } else {
                TempoChartView(readings: statusViewModel.tempoHistory)
                    .frame(height: 160)
            }
        }
    }
}

// A square selectable tile, the SwiftUI equivalent of the web client's
// RadioGroupCard program grid.
private struct ProgramTile: View {
    let name: String
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            // A fixed minimum height rather than a square aspect ratio:
            // inside a List row the height proposal is unspecified, and
            // aspectRatio collapses the tile to the text's ideal size.
            Text(name)
                .font(.callout.weight(.medium))
                .multilineTextAlignment(.center)
                .padding(8)
                .frame(maxWidth: .infinity, minHeight: 80)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(isSelected ? Color.accentColor.opacity(0.12) : Color.clear)
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .strokeBorder(
                            isSelected ? Color.accentColor : Color.secondary.opacity(0.35),
                            lineWidth: isSelected ? 1.5 : 1
                        )
                )
                .foregroundStyle(isSelected ? Color.accentColor : Color.primary)
        }
        .buttonStyle(.plain)
        .accessibilityAddTraits(isSelected ? [.isSelected] : [])
    }
}

private extension Double {
    func clamped(to range: ClosedRange<Double>) -> Double {
        Swift.min(range.upperBound, Swift.max(range.lowerBound, self))
    }
}
