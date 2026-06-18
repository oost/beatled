import SwiftUI

// Device rows mirroring the web client's Devices table: board id, IP,
// firmware self-description, and the per-device QoS columns (clock offset,
// median RTT, NEXT_BEAT gaps).
struct DevicesTableView: View {
    let devices: [Device]

    var body: some View {
        ForEach(devices) { device in
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text(device.boardId)
                        .font(.system(.body, design: .monospaced))
                    Spacer()
                    Text(device.lastSeenText)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Text(subtitle(for: device))
                    .font(.caption)
                    .foregroundStyle(.secondary)
                Text(qosLine(for: device))
                    .font(.system(.caption, design: .monospaced))
                    .foregroundStyle(.secondary)
            }
            .accessibilityElement(children: .combine)
        }
    }

    // "ip · port · sha" — firmware fields are empty for v2-or-older clients.
    private func subtitle(for device: Device) -> String {
        var parts = [device.ipAddress]
        if let port = device.portName, !port.isEmpty { parts.append(port) }
        if let sha = device.gitSha, !sha.isEmpty { parts.append(sha) }
        return parts.joined(separator: " · ")
    }

    private func qosLine(for device: Device) -> String {
        let offset = Self.signedMs(device.qos?.currentOffsetUs)
        let rtt = Self.ms(device.qos?.medianRttUs)
        let gaps = device.qos?.nextBeatGapTotal.map(String.init) ?? "—"
        return "offset \(offset) · RTT \(rtt) · NB gaps \(gaps) · up \(device.uptimeText)"
    }

    private static func ms(_ us: Double?) -> String {
        guard let us, us != 0 else { return "—" }
        return String(format: "%.1f ms", us / 1000)
    }

    private static func signedMs(_ us: Double?) -> String {
        guard let us else { return "—" }
        return String(format: "%+.1f ms", us / 1000)
    }
}
