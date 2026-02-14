import { useMemo } from "react";
import { Line } from "react-chartjs-2";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  TimeScale,
} from "chart.js";
import "chartjs-adapter-date-fns";
import { enUS } from "date-fns/locale";

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Tooltip, TimeScale);

interface HistoryPoint {
  x: Date;
  y: number;
}

const chartOptions = {
  responsive: true,
  scales: {
    x: {
      type: "time" as const,
      adapters: {
        date: {
          locale: enUS,
        },
      },
    },
  },
} as const;

export default function BeatChart({ historyData }: { historyData: HistoryPoint[] }) {
  const chartData = useMemo(
    () => ({
      datasets: [
        {
          label: "Tempo",
          borderColor: "#f96332",
          pointBorderColor: "#FFF",
          pointBackgroundColor: "#f96332",
          pointBorderWidth: 2,
          pointHoverRadius: 4,
          pointHoverBorderWidth: 1,
          pointRadius: 4,
          borderWidth: 2,
          tension: 0.4,
          data: historyData,
        },
      ],
    }),
    [historyData],
  );

  return (
    <div className="relative w-full">
      {historyData?.length ? (
        <Line data={chartData} options={chartOptions} />
      ) : (
        <p>Beat Detector Paused</p>
      )}
    </div>
  );
}
