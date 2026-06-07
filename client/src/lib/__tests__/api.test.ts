import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import {
  getEndpoint,
  postEndpoint,
  getAPIHost,
  setAPIHost,
  getAPIToken,
  setAPIToken,
  pingHealth,
} from "../api";

function createStorageMock() {
  let store: Record<string, string> = {};
  return {
    getItem: vi.fn((key: string) => store[key] ?? null),
    setItem: vi.fn((key: string, value: string) => {
      store[key] = value;
    }),
    removeItem: vi.fn((key: string) => {
      delete store[key];
    }),
    clear: vi.fn(() => {
      store = {};
    }),
    get length() {
      return Object.keys(store).length;
    },
    key: vi.fn((i: number) => Object.keys(store)[i] ?? null),
  };
}

const sessionStorageMock = createStorageMock();
const localStorageMock = createStorageMock();

Object.defineProperty(globalThis, "sessionStorage", { value: sessionStorageMock, writable: true });
Object.defineProperty(globalThis, "localStorage", { value: localStorageMock, writable: true });

describe("API token configuration", () => {
  beforeEach(() => {
    sessionStorageMock.clear();
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

  it("persists host to localStorage", () => {
    setAPIHost("https://custom-host:9090");
    expect(localStorageMock.setItem).toHaveBeenCalledWith(
      "beatled_api_host",
      "https://custom-host:9090",
    );
  });
});

describe("getEndpoint", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    sessionStorageMock.clear();
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
    sessionStorageMock.clear();
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
    sessionStorageMock.clear();
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

describe("pingHealth", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  it("hits /api/health on the given host", async () => {
    const mockResponse = { ok: true } as Response;
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse);

    const ok = await pingHealth("https://beatled.local:8443");

    expect(ok).toBe(true);
    const [url] = (fetch as Mock).mock.calls[0];
    expect(url.toString()).toBe("https://beatled.local:8443/api/health");
  });

  it("returns false on a non-2xx response", async () => {
    const mockResponse = { ok: false } as Response;
    vi.spyOn(globalThis, "fetch").mockResolvedValue(mockResponse);

    expect(await pingHealth("https://beatled.local:8443")).toBe(false);
  });

  it("returns false on a network error (never throws)", async () => {
    vi.spyOn(globalThis, "fetch").mockRejectedValue(new Error("ENETUNREACH"));

    expect(await pingHealth("https://beatled.local:8443")).toBe(false);
  });

  it("returns false when the internal timeout fires", async () => {
    // Resolve the fetch with an AbortError shaped like one the runtime would
    // throw if the controller's signal had aborted. We rely on the catch path
    // returning false rather than propagating.
    vi.spyOn(globalThis, "fetch").mockImplementation((_input, init) => {
      return new Promise((_resolve, reject) => {
        const signal = (init as RequestInit | undefined)?.signal;
        signal?.addEventListener("abort", () => reject(new Error("aborted")));
      });
    });

    expect(await pingHealth("https://beatled.local:8443", { timeoutMs: 5 })).toBe(false);
  });

  it("respects a caller-supplied abort signal", async () => {
    const controller = new AbortController();
    vi.spyOn(globalThis, "fetch").mockImplementation((_input, init) => {
      return new Promise((_resolve, reject) => {
        (init as RequestInit | undefined)?.signal?.addEventListener("abort", () =>
          reject(new Error("aborted")),
        );
      });
    });

    const pending = pingHealth("https://beatled.local:8443", { signal: controller.signal });
    controller.abort();
    expect(await pending).toBe(false);
  });
});
