function getEndpoint(endpoint) {
  return fetch(endpoint, {
    method: "GET", // *GET, POST, PUT, DELETE, etc.
    mode: "cors", // no-cors, *cors, same-origin
    cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
    credentials: "same-origin", // include, *same-origin, omit
    headers: {
      "Content-Type": "application/json",
    },
    redirect: "follow", // manual, *follow, error
    referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
  });
}

export async function getStatus() {
  const res = await getEndpoint("/api/status");
  return res.json();
}

export async function startBeatDetector() {
  const res = await getEndpoint("/api/beat-detector/start");
  return res.json();
}

export async function stopBeatDetector() {
  const res = await getEndpoint("/api/beat-detector/stop");
  return res.json();
}
