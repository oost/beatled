const API_HOST_KEY = "beatled_api_host";

let API_HOST: string | null = null;

function resolveHost(): string {
  if (API_HOST === null) {
    API_HOST = localStorage.getItem(API_HOST_KEY) ?? window.location.origin;
  }
  return API_HOST;
}

export function setAPIHost(new_host: string) {
  console.log(`Changed API_HOST to ${new_host}`);
  API_HOST = new_host;
  localStorage.setItem(API_HOST_KEY, new_host);
}

export function getAPIHost(): string {
  return resolveHost();
}

const API_TOKEN_KEY = "beatled_api_token";

export function setAPIToken(token: string) {
  if (token) {
    sessionStorage.setItem(API_TOKEN_KEY, token);
  } else {
    sessionStorage.removeItem(API_TOKEN_KEY);
  }
}

export function getAPIToken(): string {
  return sessionStorage.getItem(API_TOKEN_KEY) ?? "";
}

type HttpMethod = "GET" | "POST" | "PUT" | "DELETE";

async function executeFetch(
  endpoint: string,
  method: HttpMethod,
  body?: unknown,
): Promise<Response> {
  try {
    const response = await fetch(new URL(endpoint, resolveHost()), {
      method,
      mode: "cors",
      cache: "no-cache",
      credentials: "same-origin",
      headers: {
        "Content-Type": "application/json",
        ...(getAPIToken() && { Authorization: `Bearer ${getAPIToken()}` }),
      },
      body: body ? JSON.stringify(body) : undefined,
      redirect: "follow",
      referrerPolicy: "no-referrer",
    });
    if (!response.ok) {
      console.error(`fetch error ${response.status}: ${response.statusText}`);
      throw Error(response.statusText);
    }
    return response;
  } catch (err) {
    console.error(`Error calling ${method} ${endpoint}: ${err}`);
    throw err;
  }
}

export function getEndpoint(endpoint: string): Promise<Response> {
  return executeFetch(endpoint, "GET");
}

export function postEndpoint(endpoint: string, body?: unknown): Promise<Response> {
  return executeFetch(endpoint, "POST", body);
}

/**
 * Health-probe an arbitrary host (used by the Config view's discovery
 * scan, which is independent of the currently-configured API_HOST).
 *
 * Returns true on a 2xx, false on anything else (including network
 * error, non-2xx status, abort, or timeout). Never throws — callers
 * just want a status pill, not an exception chain.
 *
 * - `timeoutMs` defaults to 3000 to match the prior behaviour in
 *   views/config.tsx that this function replaces.
 * - `signal` lets the caller integrate with a shared cancellation
 *   token (e.g. for unmount cleanup). Internal timeout is wired
 *   through a child controller so both sources can cancel the request.
 */
export async function pingHealth(
  hostBaseUrl: string,
  options: { timeoutMs?: number; signal?: AbortSignal } = {},
): Promise<boolean> {
  const { timeoutMs = 3000, signal } = options;
  const internal = new AbortController();
  const timer = setTimeout(() => internal.abort(), timeoutMs);
  const onParentAbort = () => internal.abort();
  signal?.addEventListener("abort", onParentAbort);

  try {
    const res = await fetch(new URL("/api/health", hostBaseUrl), {
      signal: internal.signal,
      mode: "cors",
      cache: "no-cache",
    });
    return res.ok;
  } catch {
    return false;
  } finally {
    clearTimeout(timer);
    signal?.removeEventListener("abort", onParentAbort);
  }
}
