import SwiftUI

struct StatusView: View {
    @Bindable var viewModel: StatusViewModel

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

                tempoSection
                serviceTogglesSection
                tempoChartSection
                devicesSection
            }
            .navigationTitle("Status")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button { viewModel.refresh() } label: {
                        Image(systemName: "arrow.clockwise")
                    }
                    .keyboardShortcut("r", modifiers: .command)
                }
            }
            .onAppear { viewModel.startPolling() }
            .onDisappear { viewModel.stopPolling() }
        }
    }

    @ViewBuilder
    private var tempoSection: some View {
        Section("Tempo") {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(String(format: "%.1f", viewModel.status?.tempo ?? 0))
                        .font(.system(size: 48, weight: .bold, design: .rounded))
                        .accessibilityLabel("\(String(format: "%.1f", viewModel.status?.tempo ?? 0)) BPM")
                    Text("BPM")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .accessibilityHidden(true)
                }
                Spacer()
                VStack(alignment: .trailing, spacing: 4) {
                    Text("\(viewModel.devices.count)")
                        .font(.system(size: 28, weight: .semibold, design: .rounded))
                        .accessibilityLabel("\(viewModel.devices.count) devices connected")
                    Text("Devices")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .accessibilityHidden(true)
                }
            }
            .padding(.vertical, 4)

            if let lastUpdate = viewModel.lastUpdate {
                HStack {
                    Image(systemName: "clock")
                        .font(.caption2)
                    Text("Updated \(lastUpdate, style: .relative) ago")
                        .font(.caption)
                }
                .foregroundStyle(.secondary)
            }
        }
    }

    @ViewBuilder
    private var serviceTogglesSection: some View {
        if let services = viewModel.status?.status, !services.isEmpty {
            Section("Services") {
                ForEach(services.keys.sorted(), id: \.self) { key in
                    Toggle(key, isOn: Binding(
                        get: { services[key] ?? false },
                        set: { viewModel.toggleService(id: key, enabled: $0) }
                    ))
                    .toggleStyle(.switch)
                }
            }
        }
    }

    @ViewBuilder
    private var tempoChartSection: some View {
        Section("Tempo History") {
            if viewModel.tempoHistory.isEmpty {
                ContentUnavailableView {
                    Label("No Data", systemImage: "waveform.slash")
                } description: {
                    Text("Beat detector is paused or not yet running.")
                }
                .frame(height: 160)
            } else {
                TempoChartView(readings: viewModel.tempoHistory)
                    .frame(height: 160)
            }
        }
    }

    @ViewBuilder
    private var devicesSection: some View {
        Section("Devices (\(viewModel.devices.count))") {
            if viewModel.devices.isEmpty {
                ContentUnavailableView {
                    Label("No Devices", systemImage: "antenna.radiowaves.left.and.right.slash")
                } description: {
                    Text("No devices connected to the server.")
                }
            } else {
                DevicesTableView(devices: viewModel.devices)
            }
        }
    }
}
