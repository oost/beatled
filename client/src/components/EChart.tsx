import { useEffect, useRef } from "react";
import * as echarts from "echarts/core";
import { LineChart } from "echarts/charts";
import { GridComponent, TooltipComponent } from "echarts/components";
import { CanvasRenderer } from "echarts/renderers";
import type { EChartsCoreOption, EChartsType } from "echarts/core";

echarts.use([LineChart, GridComponent, TooltipComponent, CanvasRenderer]);

// Thin ECharts wrapper owning the whole echarts dependency (registration,
// init, resize, dispose) so consumers and tests never touch echarts
// directly. Options are applied with merge-mode setOption, which drives
// ECharts' update animation: series are matched by id and data items by
// name, so a shifted data window slides into place instead of redrawing.
// The container must be given an explicit height via className.
export default function EChart({
  option,
  className,
}: {
  option: EChartsCoreOption;
  className?: string;
}) {
  const containerRef = useRef<HTMLDivElement>(null);
  const chartRef = useRef<EChartsType | null>(null);

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;
    const chart = echarts.init(el);
    chartRef.current = chart;
    // Guarded: jsdom (vitest) has no ResizeObserver.
    let observer: ResizeObserver | undefined;
    if (typeof ResizeObserver !== "undefined") {
      observer = new ResizeObserver(() => chart.resize());
      observer.observe(el);
    }
    return () => {
      observer?.disconnect();
      chart.dispose();
      chartRef.current = null;
    };
  }, []);

  useEffect(() => {
    chartRef.current?.setOption(option);
  }, [option]);

  return <div ref={containerRef} className={className} />;
}
