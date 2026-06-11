// View-level test for the Config page, mirroring program.test.tsx: the
// real route declaration (loader + action + element) is mounted in a
// MemoryRouter with lib/api stubbed.

import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { createMemoryRouter, RouterProvider, type RouteObject } from "react-router-dom";

vi.mock("../../lib/api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../../lib/api")>()),
  getAPIHost: vi.fn(() => "https://localhost:8443"),
  setAPIHost: vi.fn(() => true),
  getAPIToken: vi.fn(() => ""),
  setAPIToken: vi.fn(),
  pingHealth: vi.fn(() => Promise.resolve(true)),
}));

import { setAPIHost, setAPIToken, pingHealth } from "../../lib/api";
import ConfigPage, { loader as configLoader, action as configAction } from "../config";

function renderConfigRoute() {
  const routes: RouteObject[] = [
    { path: "/config", loader: configLoader, action: configAction, element: <ConfigPage /> },
  ];
  const router = createMemoryRouter(routes, { initialEntries: ["/config"] });
  return render(<RouterProvider router={router} />);
}

describe("ConfigPage", () => {
  beforeEach(() => {
    vi.clearAllMocks();
    (pingHealth as Mock).mockResolvedValue(true);
  });

  it("renders the host options and probes each one's health", async () => {
    renderConfigRoute();

    expect(await screen.findByRole("radio", { name: /beatled\.local/ })).toBeInTheDocument();
    expect(screen.getByRole("radio", { name: /Localhost/ })).toBeInTheDocument();

    // One health probe per listed host.
    await waitFor(() => expect(pingHealth).toHaveBeenCalledTimes(5));
    expect(pingHealth).toHaveBeenCalledWith(
      "https://beatled.local:8443",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
  });

  it("stores the selected host when the user picks one", async () => {
    const user = userEvent.setup();
    renderConfigRoute();
    await screen.findByRole("radio", { name: /Raspberry Pi/ });

    await user.click(screen.getByRole("radio", { name: /Raspberry Pi/ }));

    await waitFor(() => expect(setAPIHost).toHaveBeenCalledWith("https://raspberrypi1.local:8443"));
  });

  it("auto-selects the first healthy host when the current one is down", async () => {
    (pingHealth as Mock).mockImplementation((host: string) =>
      Promise.resolve(host === "https://beatled.test:8443"),
    );
    renderConfigRoute();

    await waitFor(() => expect(setAPIHost).toHaveBeenCalledWith("https://beatled.test:8443"));
  });

  it("stores the token when the input loses focus", async () => {
    const user = userEvent.setup();
    renderConfigRoute();
    const input = await screen.findByLabelText("API Token");

    await user.type(input, "secret-token");
    await user.tab();

    await waitFor(() => expect(setAPIToken).toHaveBeenCalledWith("secret-token"));
  });
});
