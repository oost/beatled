import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen } from "@testing-library/react";
import type { EChartsCoreOption } from "echarts/core";

// jsdom has no canvas: mock the wrapper so echarts never initializes, and
// capture the option prop so tests can assert on the series data.
let lastOption: EChartsCoreOption | undefined;
vi.mock("../EChart", () => ({
  default: ({ option }: { option: EChartsCoreOption }) => {
    lastOption = option;
    return <div data-testid="mock-chart">Chart</div>;
  },
}));

// use-theme calls window.matchMedia at module load; jsdom lacks it.
vi.mock("../../hooks/use-theme", () => ({
  useTheme: () => ({ theme: "light", toggle: vi.fn() }),
}));

import BeatChart from "../BeatChart";

describe("BeatChart", () => {
  beforeEach(() => {
    lastOption = undefined;
  });

  it("renders chart when historyData is provided", () => {
    const historyData = [
      { x: new Date(1000), y: 120 },
      { x: new Date(3000), y: 125 },
    ];

    render(<BeatChart historyData={historyData} />);
    expect(screen.getByTestId("mock-chart")).toBeInTheDocument();
  });

  it("passes the points through with stable per-point identities", () => {
    const historyData = [
      { x: new Date(1000), y: 120 },
      { x: new Date(3000), y: 125 },
    ];

    render(<BeatChart historyData={historyData} />);
    const series = (lastOption as { series: Array<{ data: Array<{ name: string }> }> }).series;
    expect(series[0].data).toHaveLength(2);
    // Stable names are what make merge-mode setOption diff by timestamp,
    // which produces the slide-in animation.
    expect(series[0].data.map((d) => d.name)).toEqual(["1000", "3000"]);
  });

  it("renders paused message when historyData is falsy", () => {
    render(<BeatChart historyData={[] as never} />);
    expect(screen.getByText("Beat Detector Paused")).toBeInTheDocument();
  });
});
