import { describe, it, expect, vi, beforeEach } from "vitest";
import { getStatus, serviceControl } from "../status";

vi.mock("../api", () => ({
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
  getAPIHost: vi.fn(() => "https://localhost:8080"),
  setAPIHost: vi.fn(),
}));

import { getEndpoint, postEndpoint } from "../api";

describe("getStatus", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("returns parsed JSON on success", async () => {
    const statusData = {
      message: "It's all good!",
      status: { beat_detector: true },
      tempo: 120.5,
    };
    getEndpoint.mockResolvedValue({
      json: () => Promise.resolve(statusData),
    });

    const result = await getStatus();

    expect(getEndpoint).toHaveBeenCalledWith("/api/status");
    expect(result).toEqual(statusData);
  });

  it("returns error object on network failure", async () => {
    getEndpoint.mockRejectedValue(new Error("Connection refused"));

    const result = await getStatus();

    expect(result).toEqual({ error: true, status: "Network error" });
  });
});

describe("serviceControl", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("posts service control request", async () => {
    postEndpoint.mockResolvedValue({
      json: () => Promise.resolve({ status: true }),
    });

    const result = await serviceControl("beat_detector", true);

    expect(postEndpoint).toHaveBeenCalledWith("/api/service/control", {
      id: "beat_detector",
      status: true,
    });
    expect(result).toEqual({ status: true });
  });

  it("returns error object on failure", async () => {
    postEndpoint.mockRejectedValue(new Error("Network error"));

    const result = await serviceControl("beat_detector", true);

    expect(result).toEqual({ error: true, status: "Network error" });
  });
});
