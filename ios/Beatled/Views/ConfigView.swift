import SwiftUI

struct ConfigView: View {
    @State var viewModel: ConfigViewModel
    @State private var showInsecureWarning = false

    var body: some View {
        NavigationStack {
            Form {
                serverSection
                authenticationSection
                securitySection
                aboutSection
            }
            .formStyle(.grouped)
            .navigationTitle("Config")
            .task { await viewModel.runHealthChecks() }
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
        }
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
            LabeledContent("Version", value: "1.0")
        }
    }
}

private struct HealthDot: View {
    let status: HealthStatus

    var body: some View {
        Circle()
            .fill(color)
            .frame(width: 8, height: 8)
    }

    private var color: Color {
        switch status {
        case .ok: .green
        case .error: .red
        case .unknown: .gray
        }
    }
}
