import SwiftUI
import Charts

// Straight line segments over a continuously sliding 60-second window,
// matching the web client's chart. The TimelineView advances the x domain
// every 200ms so the line flows left instead of jumping on each 2s poll.
struct TempoChartView: View {
    let readings: [TempoReading]

    private static let windowSeconds: TimeInterval = 60
    private static let slideTick: TimeInterval = 0.2

    var body: some View {
        TimelineView(.periodic(from: .now, by: Self.slideTick)) { context in
            Chart(readings) { reading in
                LineMark(
                    x: .value("Time", reading.time),
                    y: .value("BPM", reading.bpm)
                )
                .foregroundStyle(.tint)
                .interpolationMethod(.linear)
            }
            .chartXScale(domain: context.date.addingTimeInterval(-Self.windowSeconds)...context.date)
            .chartYScale(domain: yDomain)
            .chartXAxis {
                AxisMarks(values: .stride(by: .second, count: 10)) { _ in
                    AxisGridLine()
                }
            }
            .chartYAxis {
                AxisMarks(position: .leading)
            }
        }
    }

    private var yDomain: ClosedRange<Double> {
        let bpms = readings.map(\.bpm)
        let min = (bpms.min() ?? 60) - 5
        let max = (bpms.max() ?? 180) + 5
        return min...max
    }
}
