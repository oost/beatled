import { getEndpoint, postEndpoint, toApiFailure, invalidResponse, type ApiErrorKind } from "./api";

export interface StatusResponse {
  error?: boolean;
  kind?: ApiErrorKind;
  httpStatus?: number;
  // string in error responses, boolean from /api/service/control, and a
  // service-id → running map from /api/status.
  status?: string | boolean | Record<string, boolean>;
  tempo?: number;
  // Operator-chosen BPM used by the `manual-bpm` service. Mirrors the field
  // the server returns from /api/status and /api/tempo/manual.
  manualBpm?: number;
  deviceCount?: number;
}

function isRecord(v: unknown): v is Record<string, unknown> {
  return typeof v === "object" && v !== null && !Array.isArray(v);
}

// Every /api/status field is optional on the wire, so this validates
// types rather than presence.
export function isStatusResponse(v: unknown): v is StatusResponse {
  if (!isRecord(v)) return false;
  if (v.tempo !== undefined && typeof v.tempo !== "number") return false;
  if (v.manualBpm !== undefined && typeof v.manualBpm !== "number") return false;
  if (v.deviceCount !== undefined && typeof v.deviceCount !== "number") return false;
  if (
    v.status !== undefined &&
    typeof v.status !== "string" &&
    typeof v.status !== "boolean" &&
    !isRecord(v.status)
  )
    return false;
  return true;
}

export async function getStatus(): Promise<StatusResponse> {
  try {
    const res = await getEndpoint("/api/status");
    const json: unknown = await res.json();
    return isStatusResponse(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
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

function isDevice(v: unknown): v is Device {
  return (
    isRecord(v) &&
    typeof v.client_id === "number" &&
    typeof v.board_id === "string" &&
    typeof v.ip_address === "string" &&
    typeof v.last_status_time === "number"
  );
}

export function isDevicesResponse(v: unknown): v is DevicesResponse {
  return (
    isRecord(v) &&
    typeof v.count === "number" &&
    Array.isArray(v.devices) &&
    v.devices.every(isDevice)
  );
}

export async function getDevices(): Promise<DevicesResponse> {
  try {
    const res = await getEndpoint("/api/devices");
    const json: unknown = await res.json();
    return isDevicesResponse(json) ? json : { devices: [], count: 0 };
  } catch {
    return { devices: [], count: 0 };
  }
}

// Fleet-wide QoS aggregates returned by /api/qos. Optional fields are
// null when no device has reported a QoS snapshot yet.
export interface FleetQos {
  device_count: number;
  reporting_count: number;
  min_offset_us: number | null;
  max_offset_us: number | null;
  fleet_skew_us: number | null;
  mean_rtt_us: number | null;
  min_rtt_us: number | null;
  max_rtt_us: number | null;
  slowest_device_board_id: string;
  total_next_beat_gap: number;
  total_intercore_drops: number;
  total_time_sync_outliers: number;
  // Server-side verdict driven by --qos-skew-warn-us / --qos-skew-fail-us
  // (and a non-zero drop / outlier total). React renders the matching
  // colour pip; "unknown" while the fleet has zero reporting devices.
  health: "ok" | "warn" | "fail" | "unknown";
  thresholds: { skew_warn_us: number; skew_fail_us: number };
}

export function isFleetQos(v: unknown): v is FleetQos {
  return (
    isRecord(v) &&
    typeof v.device_count === "number" &&
    typeof v.reporting_count === "number" &&
    typeof v.health === "string" &&
    ["ok", "warn", "fail", "unknown"].includes(v.health) &&
    isRecord(v.thresholds)
  );
}

export async function getQos(): Promise<FleetQos | null> {
  try {
    const res = await getEndpoint("/api/qos");
    const json: unknown = await res.json();
    return isFleetQos(json) ? json : null;
  } catch {
    return null;
  }
}

export async function serviceControl(serviceId: string, status: boolean): Promise<StatusResponse> {
  try {
    const res = await postEndpoint("/api/service/control", {
      id: serviceId,
      status,
    });
    const json: unknown = await res.json();
    return isStatusResponse(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}

// Set the operator-chosen BPM for the `manual-bpm` metronome service. Stores
// the value server-side; enabling the service is a separate serviceControl
// call (the two tempo sources are mutually exclusive on the server).
export async function setManualBpm(bpm: number): Promise<StatusResponse> {
  try {
    const res = await postEndpoint("/api/tempo/manual", { bpm });
    const json: unknown = await res.json();
    return isStatusResponse(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}
