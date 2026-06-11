import { useEffect, useMemo, useRef } from "react";
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

// Visible time window; the x axis slides so the right edge is always "now".
export const WINDOW_MS = 60_000;

// How often the window slides. Much faster than the 2s data poll so the
// motion reads as continuous.
const SCROLL_TICK_MS = 200;

// Matches --primary in index.css (violet); hardcoded because canvas colors
// can't reliably resolve oklch() CSS variables.
const ACCENT = "#8b5cf6";

// Exported so tests can assert the line style (straight segments, no dots).
export const datasetStyle = {
  label: "Tempo",
  borderColor: ACCENT,
  borderWidth: 2,
  tension: 0,
  pointRadius: 0,
  pointHoverRadius: 3,
  pointBackgroundColor: ACCENT,
} as const;

const gridColor = "rgba(148, 163, 184, 0.12)";

export default function BeatChart({ historyData }: { historyData: HistoryPoint[] }) {
  const chartRef = useRef<ChartJS<"line", HistoryPoint[]> | null>(null);

  // Slide the x window between data polls. update("none") skips animation,
  // so with `animation: false` below the slide is the only motion on screen.
  useEffect(() => {
    const tick = setInterval(() => {
      const chart = chartRef.current;
      if (!chart) return;
      const now = Date.now();
      chart.options.scales!.x!.min = now - WINDOW_MS;
      chart.options.scales!.x!.max = now;
      chart.update("none");
    }, SCROLL_TICK_MS);
    return () => clearInterval(tick);
  }, []);

  const chartData = useMemo(
    () => ({ datasets: [{ ...datasetStyle, data: historyData }] }),
    [historyData],
  );

  const chartOptions = useMemo(
    () => ({
      responsive: true,
      animation: false as const,
      scales: {
        x: {
          type: "time" as const,
          min: Date.now() - WINDOW_MS,
          max: Date.now(),
          adapters: {
            date: {
              locale: enUS,
            },
          },
          grid: { color: gridColor },
        },
        y: {
          grid: { color: gridColor },
        },
      },
    }),
    [],
  );

  return (
    <div className="relative w-full">
      {historyData?.length ? (
        <Line ref={chartRef} data={chartData} options={chartOptions} />
      ) : (
        <p>Beat Detector Paused</p>
      )}
    </div>
  );
}
