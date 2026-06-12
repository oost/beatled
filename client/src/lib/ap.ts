import { postEndpoint, toApiFailure, invalidResponse, type ApiErrorKind } from "./api";

export type ApMode = "on" | "off" | "status";

// Response from POST /api/ap. A "status" query returns `ap`; "on"/"off"
// actions return `result`/`mode`/`output`. Failures carry `error` + `status`
// (the human-readable message the views render), mirroring the other helpers.
export interface ApResponse {
  error?: boolean;
  kind?: ApiErrorKind;
  httpStatus?: number;
  ap?: "on" | "off";
  result?: string;
  mode?: string;
  output?: string;
  status?: string;
}

function isRecord(v: unknown): v is Record<string, unknown> {
  return typeof v === "object" && v !== null && !Array.isArray(v);
}

export function isApResponse(v: unknown): v is ApResponse {
  if (!isRecord(v)) return false;
  if (v.ap !== undefined && v.ap !== "on" && v.ap !== "off") return false;
  return true;
}

// Toggle the Pi's WiFi between client and access-point mode, or query it.
// `revertMinutes` only applies to "on": the Pi switches back to WiFi after
// that long (a safety net so a bad switch can't strand it). Note that "on"
// tears down the link this request travels over, so the call usually fails
// to return even though the switch succeeds — callers should treat a
// network error after an "on" as "probably switched, reconnect on the AP".
export async function apControl(mode: ApMode, revertMinutes?: number): Promise<ApResponse> {
  try {
    const body: { mode: ApMode; revertMinutes?: number } = { mode };
    if (mode === "on" && revertMinutes && revertMinutes > 0) {
      body.revertMinutes = revertMinutes;
    }
    const res = await postEndpoint("/api/ap", body);
    const json: unknown = await res.json();
    return isApResponse(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}
