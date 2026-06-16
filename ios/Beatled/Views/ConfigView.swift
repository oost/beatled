import SwiftUI

struct ConfigView: View {
    @State var viewModel: ConfigViewModel
    @State private var showInsecureWarning = false
    @State private var showAPWarning = false

    var body: some View {
        NavigationStack {
            Form {
                appearanceSection
                serverSection
                authenticationSection
                securitySection
                accessPointSection
                aboutSection
            }
            .formStyle(.grouped)
            .navigationTitle("Settings")
            .alert("Security Warning", isPresented: $showInsecureWarning) {
                Button("Cancel", role: .cancel) {
                    viewModel.settings.allowInsecureConnections = false
                }
                Button("Enable Anyway", role: .destructive) {
                    // User confirmed
                }
            } message: {
                Text("Allowing insecure connections bypasses certificate validation and may expose your data to security risks. Only enable this for development or when connecting to trusted servers with self-signed certificates.")
            }
            .alert("Switch to Hotspot?", isPresented: $showAPWarning) {
                Button("Cancel", role: .cancel) {}
                Button("Switch", role: .destructive) { viewModel.switchAPOn() }
            } message: {
                Text(
                    "This device will lose its connection to the server. Rejoin the “Beatled” WiFi and open https://192.168.4.1:8443/ to continue."
                    + (viewModel.revertMinutes > 0
                       ? " The Pi auto-reverts to WiFi after \(viewModel.revertMinutes) min."
                       : ""))
            }
        }
    }

    @ViewBuilder
    private var appearanceSection: some View {
        Section("Appearance") {
            Picker("Theme", selection: $viewModel.settings.appearance) {
                ForEach(AppearancePreference.allCases) { preference in
                    Text(preference.label).tag(preference)
                }
            }
            .pickerStyle(.segmented)
            .labelsHidden()
        }
    }

    @ViewBuilder
    private var accessPointSection: some View {
        Section {
            LabeledContent("Hotspot") {
                Text(viewModel.apStatus == "unknown" ? "—" : viewModel.apStatus)
                    .foregroundStyle(.secondary)
            }
            Stepper("Auto-revert: \(viewModel.revertMinutes) min",
                    value: $viewModel.revertMinutes, in: 0...1440)
            Button("Switch to Hotspot") { showAPWarning = true }
                .disabled(viewModel.apBusy)
            Button("Switch to WiFi") { viewModel.switchAPOff() }
                .disabled(viewModel.apBusy)
            if let message = viewModel.apMessage {
                Label(message, systemImage: "wifi.exclamationmark")
                    .font(.caption)
                    .foregroundStyle(.orange)
            }
        } header: {
            Text("Access Point")
        } footer: {
            Text("Switch the Pi between your WiFi and its own “Beatled” hotspot (192.168.4.1). The Pi has a single radio, so turning the hotspot on drops this connection — rejoin the “Beatled” network to reconnect.")
        }
        .task { await viewModel.refreshAPStatus() }
    }

    @ViewBuilder
    private var serverSection: some View {
        Section("Server") {
            Picker(selection: Binding(
                get: { viewModel.selectedPreset },
                set: { viewModel.selectedPreset = $0 }
            )) {
                ForEach(HostPreset.allCases) { preset in
                    HStack {
                        Text(preset.label)
                        if let subtitle = preset.subtitle {
                            Text(" · \(subtitle)")
                                .foregroundStyle(.secondary)
                        }
                        if preset != .custom {
                            Spacer()
                            HealthDot(status: viewModel.healthStatuses[preset] ?? .unknown)
                        }
                    }
                    .tag(preset)
                }
            } label: {
                EmptyView()
            }
            .pickerStyle(.inline)
            .labelsHidden()

            if viewModel.selectedPreset == .custom {
                TextField("Custom URL", text: $viewModel.customHost)
                    .textContentType(.URL)
                    .autocorrectionDisabled()
                    #if os(iOS)
                    .textInputAutocapitalization(.never)
                    .keyboardType(.URL)
                    #endif
            }

            LabeledContent("Current") {
                Text(viewModel.settings.apiHost)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }

    @ViewBuilder
    private var authenticationSection: some View {
        Section("Authentication") {
            SecureField("API Token", text: $viewModel.settings.apiToken)
                .autocorrectionDisabled()
                #if os(iOS)
                .textInputAutocapitalization(.never)
                #endif
        }
    }

    @ViewBuilder
    private var securitySection: some View {
        Section {
            Toggle("Allow Insecure Connections", isOn: $viewModel.settings.allowInsecureConnections)
                .toggleStyle(.switch)
                .onChange(of: viewModel.settings.allowInsecureConnections) {
                    if viewModel.settings.allowInsecureConnections {
                        showInsecureWarning = true
                    }
                }

            if viewModel.settings.allowInsecureConnections {
                Label("Bypasses certificate validation", systemImage: "exclamationmark.triangle.fill")
                    .font(.caption)
                    .foregroundStyle(.orange)
            }
        } header: {
            Text("Security")
        } footer: {
            Text("Enable this to connect to servers with self-signed certificates or invalid TLS configurations. Only use on trusted networks.")
        }
    }

    @ViewBuilder
    private var aboutSection: some View {
        Section("About") {
            LabeledContent("App", value: "Beatled")
            LabeledContent("Version") {
                Text(Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "—")
            }
        }
    }
}

private struct HealthDot: View {
    let status: HealthStatus

    var body: some View {
        Circle()
            .fill(color)
            .frame(width: 8, height: 8)
            .accessibilityLabel(accessibilityText)
    }

    private var color: Color {
        switch status {
        case .ok: .green
        case .error: .red
        case .unknown: .gray
        }
    }

    private var accessibilityText: String {
        switch status {
        case .ok: "Reachable"
        case .error: "Unreachable"
        case .unknown: "Checking"
        }
    }
}
