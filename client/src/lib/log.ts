import { getEndpoint } from "./api";

export interface LogResponse {
  error?: boolean;
  status?: string;
}

export async function getLogs(): Promise<string[] | LogResponse> {
  try {
    const res = await getEndpoint("/api/log");
    return res.json();
  } catch (_err) {
    return { error: true, status: "Network error" };
  }
}
