import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { apControl } from "../ap";

// Mock only the fetch helpers; the error-classification helpers stay real.
vi.mock("../api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../api")>()),
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
}));

import { postEndpoint, ApiError } from "../api";

describe("apControl", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("queries status and parses the ap field", async () => {
    (postEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ ap: "off" }),
    });

    const result = await apControl("status");

    expect(postEndpoint).toHaveBeenCalledWith("/api/ap", { mode: "status" });
    expect(result.ap).toBe("off");
  });

  it("includes revertMinutes when turning the hotspot on", async () => {
    (postEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ result: "ok", mode: "on" }),
    });

    await apControl("on", 10);

    expect(postEndpoint).toHaveBeenCalledWith("/api/ap", { mode: "on", revertMinutes: 10 });
  });

  it("omits revertMinutes when zero", async () => {
    (postEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ result: "ok", mode: "on" }),
    });

    await apControl("on", 0);

    expect(postEndpoint).toHaveBeenCalledWith("/api/ap", { mode: "on" });
  });

  it("maps a network error to a failure response", async () => {
    (postEndpoint as Mock).mockRejectedValue(new ApiError("network"));

    const result = await apControl("off");

    expect(result.error).toBe(true);
    expect(typeof result.status).toBe("string");
  });
});
