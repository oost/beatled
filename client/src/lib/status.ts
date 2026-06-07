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

// QoS snapshot carried in /api/devices since protocol v4 — sent
// piggy-back on TEMPO_REQUEST (every ~10 s while TEMPO_SYNCED) and on
// the optional STATUS probe. `null` when the controller hasn't yet
// reported one (v3-or-older firmware, or fresh boot before the first
// TEMPO_REQUEST).
export interface DeviceQos {
  current_offset_us: number;
  uptime_us: number;
  median_rtt_us: number;
  next_beat_gap_total: number;
  intercore_drop_total: number;
  time_sync_outlier_total: number;
  valid_sample_count: number;
  last_applied_program_seq: number;
  server_received_at_us: number;
  last_rtt_us: number;
}

export interface Device {
  client_id: number;
  board_id: string;
  ip_address: string;
  last_status_time: number;
  // Firmware self-description carried on HELLO_REQUEST since protocol v3.
  // Empty / 0 when the server is talking to a v2-or-older client.
  port_name?: string;
  git_sha?: string;
  build_time_us?: number;
  // Server-smoothed one-way delay estimate (EWMA over the controller's
  // reported owd_us_estimate values). 0 if no sample yet.
  owd_us?: number;
  qos?: DeviceQos | null;
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
