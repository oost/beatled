import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { getEndpoint, postEndpoint, getAPIHost, setAPIHost, getAPIToken, setAPIToken } from "../api";

const localStorageMock = (() => {
  let store: Record<string, string> = {};
  return {
    getItem: vi.fn((key: string) => store[key] ?? null),
    setItem: vi.fn((key: string, value: string) => { store[key] = value; }),
    removeItem: vi.fn((key: string) => { delete store[key]; }),
    clear: vi.fn(() => { store = {}; }),
    get length() { return Object.keys(store).length; },
    key: vi.fn((i: number) => Object.keys(store)[i] ?? null),
  };
})();

Object.defineProperty(globalThis, "localStorage", { value: localStorageMock, writable: true });

describe("API token configuration", () => {
  beforeEach(() => {
    localStorageMock.clear();
  });

  it("returns empty string when no token set", () => {
    expect(getAPIToken()).toBe("");
  });

  it("stores and retrieves a token", () => {
    setAPIToken("my-secret-token");
    expect(getAPIToken()).toBe("my-secret-token");
  });

  it("clears token when set to empty string", () => {
    setAPIToken("my-secret-token");
    setAPIToken("");
    expect(getAPIToken()).toBe("");
  });
});

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
    localStorageMock.clear();
    setAPIHost("https://localhost:8080");
  });

  it("makes a GET request to the correct URL", async () => {
    const mockResponse = { ok: true, json: () => Promise.resolve({ data: 1 }) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse as Response);

    await getEndpoint("/api/status");

    expect(fetch).toHaveBeenCalledOnce();
    const [url, options] = (fetch as Mock).mock.calls[0];
    expect(url.toString()).toBe("https://localhost:8080/api/status");
    expect(options.method).toBe("GET");
  });

  it("throws on non-ok response", async () => {
    const mockResponse = {
      ok: false,
      statusCode: 500,
      statusText: "Internal Server Error",
    };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse as unknown as Response);

    await expect(getEndpoint("/api/status")).rejects.toThrow("Internal Server Error");
  });

  it("throws on network error", async () => {
    vi.spyOn(globalThis, "fetch").mockRejectedValue(new Error("Network error"));

    await expect(getEndpoint("/api/status")).rejects.toThrow("Network error");
  });
});

describe("getEndpoint with auth token", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    localStorageMock.clear();
    setAPIHost("https://localhost:8080");
  });

  it("includes Authorization header when token is set", async () => {
    setAPIToken("test-token");
    const mockResponse = { ok: true, json: () => Promise.resolve({}) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse as Response);

    await getEndpoint("/api/status");

    const [, options] = (fetch as Mock).mock.calls[0];
    expect(options.headers.Authorization).toBe("Bearer test-token");
  });

  it("omits Authorization header when no token", async () => {
    const mockResponse = { ok: true, json: () => Promise.resolve({}) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse as Response);

    await getEndpoint("/api/status");

    const [, options] = (fetch as Mock).mock.calls[0];
    expect(options.headers.Authorization).toBeUndefined();
  });
});

describe("postEndpoint", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    localStorageMock.clear();
    setAPIHost("https://localhost:8080");
  });

  it("makes a POST request with JSON body", async () => {
    const mockResponse = { ok: true, json: () => Promise.resolve({}) };
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse as Response);

    await postEndpoint("/api/program", { programId: 3 });

    expect(fetch).toHaveBeenCalledOnce();
    const [url, options] = (fetch as Mock).mock.calls[0];
    expect(url.toString()).toBe("https://localhost:8080/api/program");
    expect(options.method).toBe("POST");
    expect(options.body).toBe(JSON.stringify({ programId: 3 }));
  });
});
