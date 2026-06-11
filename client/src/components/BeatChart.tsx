import { useMemo } from "react";
import { format } from "date-fns";
import type { ComposeOption } from "echarts/core";
import type { LineSeriesOption } from "echarts/charts";
import type { GridComponentOption, TooltipComponentOption } from "echarts/components";
import EChart from "./EChart";
import { useTheme } from "../hooks/use-theme";

type BeatChartOption = ComposeOption<
  LineSeriesOption | GridComponentOption | TooltipComponentOption
>;

interface HistoryPoint {
  x: Date;
  y: number;
}

// Resolved at render time so the light/dark values from index.css apply.
// Fallbacks cover jsdom (empty computed styles) and first paint.
function cssColor(name: string, fallback: string): string {
  const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
  return value || fallback;
}

export default function BeatChart({ historyData }: { historyData: HistoryPoint[] }) {
  // The theme value is only a memo key: useTheme re-renders this component
  // after the `.dark` class flips, so cssColor() reads the new palette.
  const { theme } = useTheme();

  const option = useMemo<BeatChartOption>(() => {
    void theme;
    const accent = cssColor("--primary", "#f96332");
    const axisText = cssColor("--muted-foreground", "#787878");
    const gridLine = cssColor("--border", "#e8e8e8");
    const pointBorder = cssColor("--card", "#ffffff");

    return {
      animationDuration: 300,
      animationDurationUpdate: 300,
      animationEasingUpdate: "cubicOut",
      grid: { left: 48, right: 16, top: 16, bottom: 28 },
      xAxis: {
        type: "time",
        axisLine: { lineStyle: { color: gridLine } },
        axisTick: { show: false },
        axisLabel: { color: axisText, hideOverlap: true },
        splitLine: { show: false },
      },
      yAxis: {
        type: "value",
        scale: true,
        axisLabel: { color: axisText },
        splitLine: { lineStyle: { color: gridLine } },
      },
      tooltip: {
        trigger: "axis",
        formatter: (params) => {
          const p = Array.isArray(params) ? params[0] : params;
          const [timestamp, bpm] = p.value as unknown as [number, number];
          return `<strong>${bpm.toFixed(1)} BPM</strong><br/>${format(timestamp, "HH:mm:ss")}`;
        },
      },
      series: [
        {
          id: "tempo",
          name: "Tempo",
          type: "line",
          smooth: 0.4,
          symbol: "circle",
          symbolSize: 8,
          lineStyle: { width: 2, color: accent },
          itemStyle: { color: accent, borderColor: pointBorder, borderWidth: 2 },
          // The name gives each point a stable identity, so merge-mode
          // setOption diffs by timestamp: when the ring buffer shifts,
          // retained points slide left and the new point slides in at the
          // right edge instead of the whole line re-drawing.
          data: historyData.map((p) => ({
            name: String(p.x.getTime()),
            value: [p.x.getTime(), p.y] as [number, number],
          })),
        },
      ],
    };
  }, [historyData, theme]);

  return (
    <div className="relative w-full">
      {historyData?.length ? (
        <EChart option={option} className="aspect-[2/1] w-full" />
      ) : (
        <p>Beat Detector Paused</p>
      )}
    </div>
  );
}
