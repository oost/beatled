const MAX_CONSOLE_LOGS = 500;
const consoleLogs: string[] = [];

declare global {
  interface Console {
    olog: (...args: unknown[]) => void;
  }
}

export function initializeConsole() {
  if (typeof console != "undefined")
    if (typeof console.log != "undefined") console.olog = console.log;
    else console.olog = function () {};

  console.log = function (message: unknown) {
    console.olog(message);
    consoleLogs.push(message + "\n");
    if (consoleLogs.length > MAX_CONSOLE_LOGS) {
      consoleLogs.splice(0, consoleLogs.length - MAX_CONSOLE_LOGS);
    }
  };
  console.error = console.debug = console.info = console.log;
}

export function getConsoleLogs(): string[] {
  return consoleLogs;
}
