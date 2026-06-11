// View-level test for the Program page.
//
// Covers the two interesting paths from the project review's Tier-2
// finding "React view-level tests": the loader hands data to the view
// successfully, and a radio-button selection drives the action.
//
// We mount the view inside a MemoryRouter configured with the real
// loader / action / element shape so the same route declaration that
// ships in production is exercised end-to-end. fetch is stubbed at the
// network boundary via lib/api so the test never touches the wire.

import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { createMemoryRouter, RouterProvider, type RouteObject } from "react-router-dom";

vi.mock("../../lib/api", () => ({
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
  getAPIHost: vi.fn(() => "https://localhost:8080"),
  setAPIHost: vi.fn(),
}));

import { getEndpoint, postEndpoint } from "../../lib/api";
import ProgramPage, { loader as programLoader, action as programAction } from "../program";

const programResponse = {
  programId: 1,
  programs: [
    { id: 0, name: "Snakes!" },
    { id: 1, name: "Sparkles" },
    { id: 2, name: "Greys" },
  ],
};

function renderProgramRoute() {
  const routes: RouteObject[] = [
    {
      path: "/program",
      loader: programLoader,
      action: programAction,
      element: <ProgramPage />,
    },
  ];
  const router = createMemoryRouter(routes, { initialEntries: ["/program"] });
  return render(<RouterProvider router={router} />);
}

describe("ProgramPage", () => {
  beforeEach(() => {
    vi.clearAllMocks();
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve(programResponse),
    });
    (postEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({}),
    });
  });

  it("renders every program returned by the loader", async () => {
    renderProgramRoute();
    for (const p of programResponse.programs) {
      await waitFor(() => expect(screen.getByText(p.name)).toBeInTheDocument());
    }
  });

  it("marks the currently-selected program in the radio group", async () => {
    renderProgramRoute();
    await waitFor(() => screen.getByText("Sparkles"));

    // The id=1 ("Sparkles") option should be the checked radio. Radix
    // RadioGroupItem renders a <button role="radio"> with aria-checked
    // rather than a native input, so we assert through ARIA.
    const sparkles = screen.getByRole("radio", { name: "Sparkles" });
    expect(sparkles).toHaveAttribute("aria-checked", "true");
    const snakes = screen.getByRole("radio", { name: "Snakes!" });
    expect(snakes).toHaveAttribute("aria-checked", "false");
  });

  it("POSTs /api/program when the user picks a different option", async () => {
    const user = userEvent.setup();
    renderProgramRoute();
    await waitFor(() => screen.getByText("Greys"));

    await user.click(screen.getByRole("radio", { name: "Greys" }));

    await waitFor(() => {
      expect(postEndpoint).toHaveBeenCalledWith("/api/program", { programId: 2 });
    });
  });
});
