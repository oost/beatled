import { getEndpoint, postEndpoint } from "./api";

export interface StatusResponse {
  error?: boolean;
  status?: string | Record<string, boolean>;
  tempo?: number;
  deviceCount?: number;
}

export async function getStatus(): Promise<StatusResponse> {
  try {
    const res = await getEndpoint("/api/status");
    return res.json();
  } catch {
    return { error: true, status: "Network error" };
  }
}

export async function serviceControl(serviceId: string, status: boolean): Promise<StatusResponse> {
  try {
    const res = await postEndpoint("/api/service/control", {
      id: serviceId,
      status,
    });
    return res.json();
  } catch {
    return { error: true, status: "Network error" };
  }
}
