import { useCallback, useSyncExternalStore } from "react";

type Theme = "light" | "dark";

const STORAGE_KEY = "beatled_theme";

function getSystemTheme(): Theme {
  return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

function getStoredTheme(): Theme {
  return (localStorage.getItem(STORAGE_KEY) as Theme) || getSystemTheme();
}

function applyTheme(theme: Theme) {
  document.documentElement.classList.toggle("dark", theme === "dark");
}

// Apply on load (before React hydrates)
applyTheme(getStoredTheme());

let listeners: Array<() => void> = [];

function emitChange() {
  for (const listener of listeners) {
    listener();
  }
}

function subscribe(listener: () => void) {
  listeners = [...listeners, listener];
  return () => {
    listeners = listeners.filter((l) => l !== listener);
  };
}

export function useTheme() {
  const theme = useSyncExternalStore(subscribe, getStoredTheme);

  const toggle = useCallback(() => {
    const next: Theme = getStoredTheme() === "dark" ? "light" : "dark";
    localStorage.setItem(STORAGE_KEY, next);
    applyTheme(next);
    emitChange();
  }, []);

  return { theme, toggle } as const;
}
