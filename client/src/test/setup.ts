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
