const consoleLogs = [];

export function initializeConsole() {
  if (typeof console != "undefined")
    if (typeof console.log != "undefined") console.olog = console.log;
    else console.olog = function () {};

  console.log = function (message) {
    console.olog(message);
    consoleLogs.push(message + "\n");
  };
  console.error = console.debug = console.info = console.log;
}

export function getConsoleLogs() {
  return consoleLogs;
}
