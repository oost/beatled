import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { renderHook, act } from "@testing-library/react";

describe("useTheme", () => {
  let store: Record<string, string> = {};
  let matchMediaResult = false;
  let changeHandler: (() => void) | null = null;

  beforeEach(() => {
    store = {};
    document.documentElement.classList.remove("dark");
    matchMediaResult = false;
    changeHandler = null;

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
        addEventListener: vi.fn((_event: string, handler: () => void) => {
          changeHandler = handler;
        }),
        removeEventListener: vi.fn(),
        dispatchEvent: vi.fn(),
      })),
    );
  });

  afterEach(() => {
    vi.restoreAllMocks();
    vi.resetModules();
  });

  it("defaults to the system preference, resolving light when OS prefers light", async () => {
    matchMediaResult = false;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.preference).toBe("system");
    expect(result.current.resolvedTheme).toBe("light");
    expect(document.documentElement.classList.contains("dark")).toBe(false);
  });

  it("resolves dark under the system preference when OS prefers dark", async () => {
    matchMediaResult = true;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.preference).toBe("system");
    expect(result.current.resolvedTheme).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("reads a stored explicit preference from localStorage", async () => {
    store["beatled_theme"] = "dark";
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    expect(result.current.preference).toBe("dark");
    expect(result.current.resolvedTheme).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("setTheme('dark') persists and applies the dark class", async () => {
    matchMediaResult = false;
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    act(() => {
      result.current.setTheme("dark");
    });

    expect(result.current.preference).toBe("dark");
    expect(store["beatled_theme"]).toBe("dark");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("setTheme('light') persists and removes the dark class", async () => {
    store["beatled_theme"] = "dark";
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    act(() => {
      result.current.setTheme("light");
    });

    expect(result.current.preference).toBe("light");
    expect(store["beatled_theme"]).toBe("light");
    expect(document.documentElement.classList.contains("dark")).toBe(false);
  });

  it("setTheme('system') resolves against the OS preference", async () => {
    store["beatled_theme"] = "light";
    matchMediaResult = true; // OS prefers dark
    const { useTheme } = await import("../use-theme");
    const { result } = renderHook(() => useTheme());

    act(() => {
      result.current.setTheme("system");
    });

    expect(result.current.preference).toBe("system");
    expect(store["beatled_theme"]).toBe("system");
    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });

  it("follows live OS changes while the preference is 'system'", async () => {
    matchMediaResult = false;
    const { useTheme } = await import("../use-theme");
    renderHook(() => useTheme());

    expect(document.documentElement.classList.contains("dark")).toBe(false);

    // Simulate the OS flipping to dark and firing the media-query change event.
    act(() => {
      matchMediaResult = true;
      changeHandler?.();
    });

    expect(document.documentElement.classList.contains("dark")).toBe(true);
  });
});
