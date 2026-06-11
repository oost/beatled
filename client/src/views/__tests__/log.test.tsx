// View-level test for the Log page, mirroring program.test.tsx: the
// real route declaration (loader + action + element) is mounted in a
// MemoryRouter and the network boundary is stubbed at lib/api.

import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { createMemoryRouter, RouterProvider, type RouteObject } from "react-router-dom";

vi.mock("../../lib/api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../../lib/api")>()),
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
}));

vi.mock("../../lib/console", () => ({
  getConsoleLogs: vi.fn(() => ["console line one\n"]),
}));

import { getEndpoint, ApiError } from "../../lib/api";
import LogPage, { loader as logLoader, action as logAction } from "../log";

function renderLogRoute() {
  const routes: RouteObject[] = [
    { path: "/log", loader: logLoader, action: logAction, element: <LogPage /> },
  ];
  const router = createMemoryRouter(routes, { initialEntries: ["/log"] });
  return render(<RouterProvider router={router} />);
}

describe("LogPage", () => {
  beforeEach(() => {
    vi.clearAllMocks();
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve(["server line one", "server line two"]),
    });
  });

  it("renders server and console log lines", async () => {
    renderLogRoute();

    await waitFor(() => expect(screen.getByText("server line one")).toBeInTheDocument());
    expect(screen.getByText("server line two")).toBeInTheDocument();
    expect(getEndpoint).toHaveBeenCalledWith("/api/log");
    expect(screen.getByText(/console line one/)).toBeInTheDocument();
  });

  it("shows the classified error message when /api/log fails", async () => {
    (getEndpoint as Mock).mockRejectedValue(new ApiError("http", 401));
    renderLogRoute();

    await waitFor(() => expect(screen.getByText("HTTP 401 — check API token")).toBeInTheDocument());
  });

  it("toggles auto-refresh via the pause button", async () => {
    const user = userEvent.setup();
    renderLogRoute();
    await waitFor(() => screen.getByText("server line one"));

    await user.click(screen.getByRole("button", { name: "Pause auto-refresh" }));
    expect(screen.getByRole("button", { name: "Resume auto-refresh" })).toBeInTheDocument();
  });
});
