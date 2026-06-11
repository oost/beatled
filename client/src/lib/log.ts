import { getEndpoint, toApiFailure, invalidResponse, type ApiErrorKind } from "./api";

export interface LogResponse {
  error?: boolean;
  kind?: ApiErrorKind;
  httpStatus?: number;
  status?: string;
}

// /api/log returns a bare JSON array of log lines.
function isLogLines(v: unknown): v is string[] {
  return Array.isArray(v) && v.every((line) => typeof line === "string");
}

export async function getLogs(): Promise<string[] | LogResponse> {
  try {
    const res = await getEndpoint("/api/log");
    const json: unknown = await res.json();
    return isLogLines(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}
