import { describe, it, expect, vi } from "vitest";
import { render, screen } from "@testing-library/react";

// Mock chart.js and react-chartjs-2 to avoid canvas rendering issues
vi.mock("react-chartjs-2", () => ({
  Line: (props) => <div data-testid="mock-chart">Chart</div>,
}));

vi.mock("chart.js", () => ({
  Chart: { register: vi.fn() },
  CategoryScale: class {},
  LinearScale: class {},
  PointElement: class {},
  LineElement: class {},
  Tooltip: class {},
  TimeScale: class {},
}));

vi.mock("chartjs-adapter-date-fns", () => ({}));

import BeatChart from "../BeatChart";

describe("BeatChart", () => {
  it("renders chart when historyData is provided", () => {
    const historyData = [
      { x: new Date(), y: 120 },
      { x: new Date(), y: 125 },
    ];

    render(<BeatChart historyData={historyData} />);
    expect(screen.getByTestId("mock-chart")).toBeInTheDocument();
  });

  it("renders paused message when historyData is falsy", () => {
    render(<BeatChart historyData={null} />);
    expect(screen.getByText("Beat Detector Paused")).toBeInTheDocument();
  });
});
