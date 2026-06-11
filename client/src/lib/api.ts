const API_HOST_KEY = "beatled_api_host";

let API_HOST: string | null = null;

function resolveHost(): string {
  if (API_HOST === null) {
    API_HOST = localStorage.getItem(API_HOST_KEY) ?? window.location.origin;
  }
  return API_HOST;
}

// Reject anything that isn't an absolute http(s) URL — a bad value here
// would poison every later fetch (or worse, smuggle in a javascript:
// scheme). Returns false so callers can ignore invalid submissions.
export function setAPIHost(new_host: string): boolean {
  let url: URL;
  try {
    url = new URL(new_host);
  } catch {
    console.error(`Rejected API host ${new_host}: not a valid URL`);
    return false;
  }
  if (url.protocol !== "http:" && url.protocol !== "https:") {
    console.error(`Rejected API host ${new_host}: scheme must be http or https`);
    return false;
  }
  console.log(`Changed API_HOST to ${new_host}`);
  API_HOST = new_host;
  localStorage.setItem(API_HOST_KEY, new_host);
  return true;
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

// Every endpoint the client talks to. Extend the union when the server
// grows a new route; add a template-literal member if one ever takes a
// query string.
export type ApiEndpoint =
  | "/api/status"
  | "/api/program"
  | "/api/devices"
  | "/api/qos"
  | "/api/health"
  | "/api/log"
  | "/api/service/control"
  | "/api/tempo/manual";

export type ApiErrorKind = "http" | "network" | "timeout" | "invalid";

export class ApiError extends Error {
  constructor(
    readonly kind: ApiErrorKind,
    readonly status?: number,
    message?: string,
  ) {
    super(message ?? kind);
    this.name = "ApiError";
  }
}

// Failure shape the lib/* helpers return in place of a parsed response.
// `status` is the human-readable message the views already render;
// `httpStatus` carries the raw code for callers that want it.
export interface ApiFailure {
  error: true;
  kind: ApiErrorKind;
  httpStatus?: number;
  status: string;
}

export function toApiFailure(err: unknown): ApiFailure {
  if (err instanceof ApiError) {
    switch (err.kind) {
      case "http": {
        const hint = err.status === 401 || err.status === 403 ? " — check API token" : "";
        return {
          error: true,
          kind: "http",
          httpStatus: err.status,
          status: `HTTP ${err.status}${hint}`,
        };
      }
      case "timeout":
        return { error: true, kind: "timeout", status: "Request timed out" };
      default:
        break;
    }
  }
  return { error: true, kind: "network", status: "Network error" };
}

export function invalidResponse(): ApiFailure {
  return { error: true, kind: "invalid", status: "Invalid server response" };
}

const REQUEST_TIMEOUT_MS = 10_000;

async function executeFetch(
  endpoint: ApiEndpoint,
  method: HttpMethod,
  body?: unknown,
): Promise<Response> {
  // Hard per-request timeout with its own controller, independent of
  // any caller-side abort-on-unmount signalling.
  const controller = new AbortController();
  let timedOut = false;
  const timer = setTimeout(() => {
    timedOut = true;
    controller.abort();
  }, REQUEST_TIMEOUT_MS);
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
      signal: controller.signal,
    });
    if (!response.ok) {
      throw new ApiError("http", response.status, `HTTP ${response.status} ${response.statusText}`);
    }
    return response;
  } catch (err) {
    console.error(`Error calling ${method} ${endpoint}: ${err}`);
    if (err instanceof ApiError) throw err;
    if (timedOut) {
      throw new ApiError("timeout", undefined, `Timed out after ${REQUEST_TIMEOUT_MS} ms`);
    }
    throw new ApiError("network", undefined, err instanceof Error ? err.message : String(err));
  } finally {
    clearTimeout(timer);
  }
}

export function getEndpoint(endpoint: ApiEndpoint): Promise<Response> {
  return executeFetch(endpoint, "GET");
}

export function postEndpoint(endpoint: ApiEndpoint, body?: unknown): Promise<Response> {
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
