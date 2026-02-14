import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { renderHook, act } from "@testing-library/react";

describe("useTheme", () => {
  let store: Record<string, string> = {};
  let matchMediaResult = false;

  beforeEach(() => {
    store = {};
    document.documentElement.classList.remove("dark");
    matchMediaResult = false;

    vi.stubGlobal("localStorage", {
      getItem: vi.fn((key: string) => store[key] ?? null),
      setItem: vi.fn((key: string, value: string) => {
        store[key] = value;
      }),
      removeItem: vi.fn((key: string) => {
        delete store[key];
      }),
    });

    vi.stubGlobal(
      "matchMedia",
      vi.fn((query: string) => ({
        matches: matchMediaResult,
        media: query,
        addEventListener: vi.fn(),
        removeEventListener: vi.fn(),
        dispatchEvent: vi.fn(),
      })),
    );
  });

  afterEach(() => {
    vi.restoreAllMocks();
    vi.resetModules();
  });

  it("defaults to light when system prefers light", async () => {
    matchMediaResult = false;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.theme).toBe("light");
    expect(document.documentElement.classList.contains("dark")).toBe(false);
  });

  it("defaults to dark when system prefers dark", async () => {
    matchMediaResult = true;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.theme).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("reads stored theme from localStorage", async () => {
    store["beatled_theme"] = "dark";
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.theme).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("toggles from light to dark", async () => {
    matchMediaResult = false;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    act(() => {
      result.current.toggle();
    });

    expect(result.current.theme).toBe("dark");
    expect(store["beatled_theme"]).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("toggles from dark to light", async () => {
    store["beatled_theme"] = "dark";
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    act(() => {
      result.current.toggle();
    });

    expect(result.current.theme).toBe("light");
    expect(store["beatled_theme"]).toBe("light");
    expect(document.documentElement.classList.contains("dark")).toBe(false);
  });
});
