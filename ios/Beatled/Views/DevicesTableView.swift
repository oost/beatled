import SwiftUI

struct DevicesTableView: View {
    let devices: [Device]

    var body: some View {
        ForEach(devices) { device in
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text(device.board_id)
                        .font(.system(.body, design: .monospaced))
                    Text(device.ip_address)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                Text(device.lastSeenText)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}
