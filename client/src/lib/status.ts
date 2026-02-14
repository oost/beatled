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

export interface Device {
  client_id: number;
  board_id: string;
  ip_address: string;
  last_status_time: number;
}

export interface DevicesResponse {
  devices: Device[];
  count: number;
}

export async function getDevices(): Promise<DevicesResponse> {
  try {
    const res = await getEndpoint("/api/devices");
    return res.json();
  } catch {
    return { devices: [], count: 0 };
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
