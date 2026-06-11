// View-level test for the Program page.
//
// Covers the two interesting paths from the project review's Tier-2
// finding "React view-level tests": the loader hands data to the view
// successfully, and tile/slider interactions drive the right POSTs.
//
// We mount the view inside a MemoryRouter configured with the real
// loader / action / element shape so the same route declaration that
// ships in production is exercised end-to-end. fetch is stubbed at the
// network boundary via lib/api so the test never touches the wire.

import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import {
  createMemoryRouter,
  RouterProvider,
  type RouteObject,
} from "react-router-dom";
import { IntlProvider } from "react-intl";

vi.mock("../../lib/api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../../lib/api")>()),
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
}));

import { getEndpoint, postEndpoint } from "../../lib/api";
import ProgramPage, {
  loader as programLoader,
  action as programAction,
} from "../program";

const programResponse = {
  programId: 1,
  programs: [
    { id: 0, name: "Snakes!" },
    { id: 1, name: "Sparkles" },
    { id: 2, name: "Greys" },
  ],
};

const beatTrackingStatus = {
  status: { "beat-detector": true, "manual-bpm": false },
  tempo: 120,
  manualBpm: 120,
};

const manualStatus = {
  status: { "beat-detector": false, "manual-bpm": true },
  tempo: 120,
  manualBpm: 120,
};

function mockStatus(statusResponse: object) {
  (getEndpoint as Mock).mockImplementation((url: string) =>
    Promise.resolve({
      json: () =>
        Promise.resolve(url === "/api/status" ? statusResponse : programResponse),
    }),
  );
}

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
  // IntlProvider mirrors the production wrapper in index.tsx; the view
  // renders FormattedNumber once the mocked status carries a tempo.
  return render(
    <IntlProvider locale="en" defaultLocale="en">
      <RouterProvider router={router} />
    </IntlProvider>,
  );
}

describe("ProgramPage", () => {
  beforeEach(() => {
    vi.clearAllMocks();
    mockStatus(beatTrackingStatus);
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

  it("marks the currently-selected program tile", async () => {
    renderProgramRoute();
    await waitFor(() => screen.getByText("Sparkles"));

    // The id=1 ("Sparkles") tile should be the checked radio. RadioGroupCard
    // renders a <button role="radio"> with aria-checked rather than a native
    // input, so we assert through ARIA.
    const sparkles = screen.getByRole("radio", { name: "Sparkles" });
    expect(sparkles).toHaveAttribute("aria-checked", "true");
    const snakes = screen.getByRole("radio", { name: "Snakes!" });
    expect(snakes).toHaveAttribute("aria-checked", "false");
  });

  it("POSTs /api/program when the user picks a different tile", async () => {
    const user = userEvent.setup();
    renderProgramRoute();
    await waitFor(() => screen.getByText("Greys"));

    await user.click(screen.getByRole("radio", { name: "Greys" }));

    await waitFor(() => {
      expect(postEndpoint).toHaveBeenCalledWith("/api/program", { programId: 2 });
    });
  });

  it("switching to Manual enables manual-bpm and disables the beat detector", async () => {
    const user = userEvent.setup();
    renderProgramRoute();
    await waitFor(() => screen.getByRole("radio", { name: "Manual" }));

    await user.click(screen.getByRole("radio", { name: "Manual" }));

    await waitFor(() => {
      expect(postEndpoint).toHaveBeenCalledWith("/api/service/control", {
        id: "manual-bpm",
        status: true,
      });
      expect(postEndpoint).toHaveBeenCalledWith("/api/service/control", {
        id: "beat-detector",
        status: false,
      });
    });
  });

  it("hides the BPM slider in beat-tracking mode", async () => {
    renderProgramRoute();
    await waitFor(() => screen.getByRole("radio", { name: "Beat tracking" }));
    expect(screen.queryByRole("slider")).not.toBeInTheDocument();
  });

  it("shows the slider in manual mode and POSTs the BPM on commit", async () => {
    mockStatus(manualStatus);
    const user = userEvent.setup();
    renderProgramRoute();

    const slider = await screen.findByRole("slider");
    // Radix fires onValueChange and onValueCommit per keyboard step.
    slider.focus();
    await user.keyboard("{ArrowRight}");

    await waitFor(() => {
      expect(postEndpoint).toHaveBeenCalledWith("/api/tempo/manual", { bpm: 121 });
    });
  });
});
