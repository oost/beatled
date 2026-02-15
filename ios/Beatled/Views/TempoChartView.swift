import SwiftUI
import Charts

struct TempoChartView: View {
    let readings: [TempoReading]

    var body: some View {
        Chart(readings) { reading in
            LineMark(
                x: .value("Time", reading.time),
                y: .value("BPM", reading.bpm)
            )
            .foregroundStyle(.tint)
            .interpolationMethod(.catmullRom)
        }
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

    private var yDomain: ClosedRange<Double> {
        let bpms = readings.map(\.bpm)
        let min = (bpms.min() ?? 60) - 5
        let max = (bpms.max() ?? 180) + 5
        return min...max
    }
}
