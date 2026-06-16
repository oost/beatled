import { useCallback, useSyncExternalStore } from "react";

export type ThemePreference = "light" | "dark" | "system";
type ResolvedTheme = "light" | "dark";

const STORAGE_KEY = "beatled_theme";

function prefersDark(): boolean {
  return (
    typeof window !== "undefined" &&
    typeof window.matchMedia === "function" &&
    window.matchMedia("(prefers-color-scheme: dark)").matches
  );
}

function getStoredPreference(): ThemePreference {
  const stored = localStorage.getItem(STORAGE_KEY);
  // Anything legacy/unknown (the old binary store only ever wrote
  // "light"/"dark", which still validate here) falls back to "system".
  return stored === "light" || stored === "dark" || stored === "system"
    ? stored
    : "system";
}

function resolveTheme(preference: ThemePreference): ResolvedTheme {
  if (preference === "system") return prefersDark() ? "dark" : "light";
  return preference;
}

function applyTheme(preference: ThemePreference) {
  document.documentElement.classList.toggle(
    "dark",
    resolveTheme(preference) === "dark",
  );
}

// Apply on load (before React hydrates)
applyTheme(getStoredPreference());

let listeners: Array<() => void> = [];

function emitChange() {
  for (const listener of listeners) {
    listener();
  }
}

// While the preference is "system", track live OS theme changes so the
// document class follows them without a reload. The stored preference doesn't
// change, so subscribers won't re-render — but applyTheme keeps the DOM in
// sync, which is what actually drives the visible theme.
if (typeof window !== "undefined" && typeof window.matchMedia === "function") {
  window
    .matchMedia("(prefers-color-scheme: dark)")
    .addEventListener("change", () => {
      if (getStoredPreference() === "system") {
        applyTheme("system");
        emitChange();
      }
    });
}

function subscribe(listener: () => void) {
  listeners = [...listeners, listener];
  return () => {
    listeners = listeners.filter((l) => l !== listener);
  };
}

export function useTheme() {
  const preference = useSyncExternalStore(subscribe, getStoredPreference);

  const setTheme = useCallback((next: ThemePreference) => {
    localStorage.setItem(STORAGE_KEY, next);
    applyTheme(next);
    emitChange();
  }, []);

  return { preference, resolvedTheme: resolveTheme(preference), setTheme } as const;
}
