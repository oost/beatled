import { describe, it, expect, vi, beforeEach } from "vitest";
import { getEndpoint, postEndpoint, getAPIHost, setAPIHost } from "../api";

describe("API host configuration", () => {
  it("returns default API host based on window.location", () => {
    const host = getAPIHost();
    expect(host).toBe(window.location.origin);
  });

  it("allows setting and getting a custom API host", () => {
    setAPIHost("https://custom-host:9090");
    expect(getAPIHost()).toBe("https://custom-host:9090");
  });
});

describe("getEndpoint", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    setAPIHost("https://localhost:8080");
  });

  it("makes a GET request to the correct URL", async () => {
    const mockResponse = { ok: true, json: () => Promise.resolve({ data: 1 }) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse);

    await getEndpoint("/api/status");

    expect(fetch).toHaveBeenCalledOnce();
    const [url, options] = fetch.mock.calls[0];
    expect(url.toString()).toBe("https://localhost:8080/api/status");
    expect(options.method).toBe("GET");
  });

  it("throws on non-ok response", async () => {
    const mockResponse = {
      ok: false,
      statusCode: 500,
      statusText: "Internal Server Error",
    };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse);

    await expect(getEndpoint("/api/status")).rejects.toThrow(
      "Internal Server Error"
    );
  });

  it("throws on network error", async () => {
    vi.spyOn(globalThis, "fetch").mockRejectedValue(new Error("Network error"));

    await expect(getEndpoint("/api/status")).rejects.toThrow("Network error");
  });
});

describe("postEndpoint", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    setAPIHost("https://localhost:8080");
  });

  it("makes a POST request with JSON body", async () => {
    const mockResponse = { ok: true, json: () => Promise.resolve({}) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse);

    await postEndpoint("/api/program", { programId: 3 });

    expect(fetch).toHaveBeenCalledOnce();
    const [url, options] = fetch.mock.calls[0];
    expect(url.toString()).toBe("https://localhost:8080/api/program");
    expect(options.method).toBe("POST");
    expect(options.body).toBe(JSON.stringify({ programId: 3 }));
  });
});
