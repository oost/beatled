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
                fleetQosSection
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

    // Fleet-wide QoS aggregates, mirroring the web client's Fleet QoS card.
    // The server computes the health verdict from operator-tuned thresholds;
    // the app only renders the matching colour/label so the single source of
    // truth stays server-side.
    @ViewBuilder
    private var fleetQosSection: some View {
        Section {
            qosRow("Health") {
                HStack(spacing: 6) {
                    Circle()
                        .fill(healthColor)
                        .frame(width: 8, height: 8)
                    Text(healthLabel)
                }
            }
            qosRow("Reporting") {
                Text(viewModel.qos.map { "\($0.reportingCount) / \($0.deviceCount)" } ?? "—")
            }
            qosRow("Fleet skew") { Text(Self.usToMs(viewModel.qos?.fleetSkewUs)) }
            qosRow("Mean RTT") { Text(Self.usToMs(viewModel.qos?.meanRttUs)) }
            qosRow("Slowest device") { Text(slowestDeviceText) }
            qosRow("NEXT_BEAT gaps") {
                Text(viewModel.qos.map { String($0.totalNextBeatGap) } ?? "—")
            }
            qosRow("Intercore drops") {
                Text(viewModel.qos.map { String($0.totalIntercoreDrops) } ?? "—")
            }
            qosRow("TIME outliers") {
                Text(viewModel.qos.map { String($0.totalTimeSyncOutliers) } ?? "—")
            }
        } header: {
            Text("Fleet QoS")
        }
    }

    private func qosRow(_ label: String, @ViewBuilder value: () -> some View) -> some View {
        HStack {
            Text(label)
            Spacer()
            value()
                .font(.system(.caption, design: .monospaced))
                .foregroundStyle(.secondary)
        }
    }

    private var healthColor: Color {
        switch viewModel.qos?.health {
        case "ok": .green
        case "warn": .orange
        case "fail": .red
        default: .gray
        }
    }

    private var healthLabel: String {
        switch viewModel.qos?.health {
        case "ok": "Healthy"
        case "warn": "Marginal"
        case "fail": "Degraded"
        default:
            (viewModel.qos?.reportingCount ?? 0) > 0 ? "—" : "No QoS samples yet"
        }
    }

    private var slowestDeviceText: String {
        guard let qos = viewModel.qos, !qos.slowestDeviceBoardId.isEmpty else { return "—" }
        return "\(qos.slowestDeviceBoardId) (\(Self.usToMs(qos.maxRttUs)))"
    }

    private static func usToMs(_ us: Double?) -> String {
        guard let us else { return "—" }
        return String(format: "%.1f ms", us / 1000)
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
