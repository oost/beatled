import "@testing-library/jest-dom";

// jsdom has no ResizeObserver; Radix Slider (and other Radix primitives that
// measure themselves) need at least a no-op implementation to mount.
class ResizeObserverStub {
  observe() {}
  unobserve() {}
  disconnect() {}
}

if (typeof globalThis.ResizeObserver === "undefined") {
  globalThis.ResizeObserver = ResizeObserverStub as unknown as typeof ResizeObserver;
}

// The theme hook (now imported by the Settings view) reads localStorage at
// module load. This environment's localStorage isn't a working Storage (its
// methods aren't callable), so provide a minimal in-memory implementation.
if (
  typeof window !== "undefined" &&
  typeof window.localStorage?.getItem !== "function"
) {
  const mem = new Map<string, string>();
  const stub: Storage = {
    get length() {
      return mem.size;
    },
    clear: () => mem.clear(),
    getItem: (key: string) => mem.get(key) ?? null,
    key: (index: number) => Array.from(mem.keys())[index] ?? null,
    removeItem: (key: string) => {
      mem.delete(key);
    },
    setItem: (key: string, value: string) => {
      mem.set(key, String(value));
    },
  };
  Object.defineProperty(window, "localStorage", {
    value: stub,
    configurable: true,
  });
}

// jsdom has no matchMedia; the theme hook (now imported by the Settings view)
// queries prefers-color-scheme at module load. Provide a light-mode stub so
// any view that pulls in useTheme can mount. The dedicated use-theme test
// overrides this with its own controllable mock.
if (typeof window !== "undefined" && typeof window.matchMedia !== "function") {
  window.matchMedia = ((query: string) => ({
    matches: false,
    media: query,
    onchange: null,
    addEventListener: () => {},
    removeEventListener: () => {},
    addListener: () => {},
    removeListener: () => {},
    dispatchEvent: () => false,
  })) as unknown as typeof window.matchMedia;
}
