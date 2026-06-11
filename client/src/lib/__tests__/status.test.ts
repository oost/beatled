import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { getStatus, getDevices, serviceControl } from "../status";

// Mock only the fetch helpers; the error-classification helpers
// (ApiError, toApiFailure, ...) stay real.
vi.mock("../api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../api")>()),
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
}));

import { getEndpoint, postEndpoint, ApiError } from "../api";

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
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve(statusData),
    });

    const result = await getStatus();

    expect(getEndpoint).toHaveBeenCalledWith("/api/status");
    expect(result).toEqual(statusData);
  });

  it("returns error object on network failure", async () => {
    (getEndpoint as Mock).mockRejectedValue(new Error("Connection refused"));

    const result = await getStatus();

    expect(result).toEqual({ error: true, kind: "network", status: "Network error" });
  });

  it("carries the HTTP status code through on http failure", async () => {
    (getEndpoint as Mock).mockRejectedValue(new ApiError("http", 401));

    const result = await getStatus();

    expect(result).toEqual({
      error: true,
      kind: "http",
      httpStatus: 401,
      status: "HTTP 401 — check API token",
    });
  });

  it("treats a malformed response body as an error", async () => {
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve("not an object"),
    });

    const result = await getStatus();

    expect(result).toEqual({ error: true, kind: "invalid", status: "Invalid server response" });
  });
});

describe("getDevices", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("returns parsed devices on success", async () => {
    const devicesData = {
      devices: [
        {
          client_id: 1,
          board_id: "BEAD5058",
          ip_address: "192.168.1.10",
          last_status_time: 1700000000000000,
        },
      ],
      count: 1,
    };
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve(devicesData),
    });

    const result = await getDevices();

    expect(getEndpoint).toHaveBeenCalledWith("/api/devices");
    expect(result).toEqual(devicesData);
  });

  it("returns empty devices on network failure", async () => {
    (getEndpoint as Mock).mockRejectedValue(new Error("Connection refused"));

    const result = await getDevices();

    expect(result).toEqual({ devices: [], count: 0 });
  });

  it("returns empty devices when the response shape is wrong", async () => {
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ devices: [{ board_id: 42 }], count: 1 }),
    });

    const result = await getDevices();

    expect(result).toEqual({ devices: [], count: 0 });
  });
});

describe("serviceControl", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("posts service control request", async () => {
    (postEndpoint as Mock).mockResolvedValue({
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
    (postEndpoint as Mock).mockRejectedValue(new Error("Network error"));

    const result = await serviceControl("beat_detector", true);

    expect(result).toEqual({ error: true, kind: "network", status: "Network error" });
  });
});
