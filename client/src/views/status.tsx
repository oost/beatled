import { useFetcher } from "react-router-dom";
import { useEffect, useState } from "react";
import { useInterval } from "../hooks/interval";
import { getStatus, getDevices, getQos, serviceControl, type Device, type FleetQos } from "../lib/status";
import PageHeader from "../components/page-header";
import { Card, CardContent, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Switch } from "@/components/ui/switch";
import { Button } from "@/components/ui/button";
import { FormattedNumber } from "react-intl";
import { format, formatDistanceToNow } from "date-fns";
import { RefreshCw, Clock } from "lucide-react";

interface TempoHistoryEntry {
  x: Date;
  y: number;
  error?: boolean;
  status?: string | boolean | Record<string, boolean>;
  tempo?: number;
  deviceCount?: number;
  uptime_us?: number;
}

// Compact duration: "3d 4h", "4h 12m", "12m 5s", "5s".
function formatDurationUs(us: number): string {
  const totalSec = Math.floor(us / 1_000_000);
  const d = Math.floor(totalSec / 86400);
  const h = Math.floor((totalSec % 86400) / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const s = totalSec % 60;
  if (d > 0) return `${d}d ${h}h`;
  if (h > 0) return `${h}h ${m}m`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

// Controller time-since-boot, reported in the QoS block (uptime_us).
function formatDeviceUptime(device: Device): string {
  const us = device.qos?.uptime_us;
  if (us === undefined || us === null) return "—";
  return formatDurationUs(us);
}

export async function loader() {
  console.log("Loading status");
  const [status, devicesResponse, qos] = await Promise.all([
    getStatus(),
    getDevices(),
    getQos(),
  ]);
  const last: TempoHistoryEntry = { x: new Date(), y: status.tempo || NaN, ...status };
  return {
    last,
    devices: devicesResponse.devices,
    qos,
  };
}

export async function action() {
  return true;
}

function formatLastSeen(lastStatusTimeUs: number): string {
  if (!lastStatusTimeUs) return "Never";
  const date = new Date(lastStatusTimeUs / 1000);
  return formatDistanceToNow(date, { addSuffix: true });
}

// Render the firmware self-description: "port · sha · built X ago".
// Empty when none of the three fields are populated — that's a v2 client
// or a server build that didn't capture them.
function formatFirmware(device: Device): string {
  const parts: string[] = [];
  if (device.port_name) parts.push(device.port_name);
  if (device.git_sha) parts.push(device.git_sha);
  if (device.build_time_us && device.build_time_us > 0) {
    parts.push("built " + formatDistanceToNow(new Date(device.build_time_us / 1000), { addSuffix: true }));
  }
  return parts.length === 0 ? "—" : parts.join(" · ");
}

// Controller clock offset relative to the server, in milliseconds. The
// server stores it microseconds and we want a one-decimal display.
function formatOffset(device: Device): string {
  const us = device.qos?.current_offset_us;
  if (us === undefined || us === null) return "—";
  const ms = us / 1000;
  const sign = ms > 0 ? "+" : "";
  return `${sign}${ms.toFixed(1)} ms`;
}

// Median round-trip time as reported by the controller's sliding-window
// time-sync filter.
function formatRtt(device: Device): string {
  const us = device.qos?.median_rtt_us;
  if (us === undefined || us === null || us === 0) return "—";
  return `${(us / 1000).toFixed(1)} ms`;
}

// Cumulative NEXT_BEAT seq gap count since boot. The controller doesn't
// know the total broadcasts sent so we show the raw count — operators
// looking for a "rate" can compare against uptime.
function formatLoss(device: Device): string {
  const gaps = device.qos?.next_beat_gap_total;
  if (gaps === undefined || gaps === null) return "—";
  return `${gaps}`;
}

function usToMs(value: number | null | undefined): string {
  if (value === null || value === undefined) return "—";
  return `${(value / 1000).toFixed(1)} ms`;
}

interface FleetQosCardProps {
  qos: FleetQos | null;
}

// Compact fleet-wide QoS card rendered next to the Devices table. Reads
// /api/qos; renders aggregates that the per-row Devices columns can't
// express on their own (fleet skew, mean RTT, slowest device).
function FleetQosCard({ qos }: FleetQosCardProps) {
  // The server computes the verdict from operator-tuned thresholds and
  // returns it in `qos.health` — React only renders the matching
  // colour/label so a single source of truth stays on the server.
  let pip = "bg-muted";
  let label = "—";
  if (qos) {
    switch (qos.health) {
      case "ok":
        pip = "bg-green-500";
        label = "Healthy";
        break;
      case "warn":
        pip = "bg-amber-500";
        label = "Marginal";
        break;
      case "fail":
        pip = "bg-red-500";
        label = "Degraded";
        break;
      case "unknown":
      default:
        label = qos.reporting_count > 0 ? "—" : "No QoS samples yet";
    }
  }
  return (
    <Card className="h-full">
      <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-3">
        <CardTitle className="text-base">Fleet QoS</CardTitle>
        <span className="flex items-center gap-2 text-xs text-muted-foreground">
          <span className={`inline-block h-2 w-2 rounded-full ${pip}`} aria-hidden />
          {label}
        </span>
      </CardHeader>
      <CardContent>
        <Table>
          <TableBody>
            <TableRow>
              <TableCell className="font-medium">Reporting</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {qos ? `${qos.reporting_count} / ${qos.device_count}` : "—"}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">Fleet skew</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {usToMs(qos?.fleet_skew_us ?? null)}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">Mean RTT</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {usToMs(qos?.mean_rtt_us ?? null)}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">Slowest device</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {qos && qos.slowest_device_board_id
                  ? `${qos.slowest_device_board_id} (${usToMs(qos.max_rtt_us)})`
                  : "—"}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">NEXT_BEAT gaps</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {qos ? qos.total_next_beat_gap : "—"}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">Intercore drops</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {qos ? qos.total_intercore_drops : "—"}
              </TableCell>
            </TableRow>
            <TableRow>
              <TableCell className="font-medium">TIME outliers</TableCell>
              <TableCell className="text-right font-mono text-xs">
                {qos ? qos.total_time_sync_outliers : "—"}
              </TableCell>
            </TableRow>
          </TableBody>
        </Table>
      </CardContent>
    </Card>
  );
}

export default function StatusPage() {
  const fetcher = useFetcher();

  // Drives the live server-uptime display. The status data itself is polled
  // by the 2s fetcher below; this 30s tick re-renders so the uptime keeps
  // counting up between polls (and refreshes at least every 30s, per spec).
  const [now, setNow] = useState(() => Date.now());
  useInterval(() => setNow(Date.now()), 30 * 1000);

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit(null);
    }
  }, 2 * 1000);

  const toggleService = async (serviceId: string, status: boolean) => {
    await serviceControl(serviceId, status);
    return fetcher.submit(null);
  };

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit(null);
    }
  }, [fetcher]);

  const fetcherData = fetcher.data as
    | {
        last: TempoHistoryEntry;
        devices: Device[];
        qos: FleetQos | null;
      }
    | undefined;
  const data = fetcherData?.last || ({} as Partial<TempoHistoryEntry>);
  const devices = fetcherData?.devices || [];
  const qos = fetcherData?.qos ?? null;

  // Server "time since last restart". uptime_us is sampled at fetch time
  // (data.x); extrapolate to the current `now` tick so it counts up live.
  const serverUptime =
    data.uptime_us !== undefined && data.x
      ? formatDurationUs(data.uptime_us + Math.max(0, now - data.x.getTime()) * 1000)
      : null;

  return (
    <>
      <PageHeader title="Status" />
      <div className="grid gap-4 px-4 pb-8 md:grid-cols-2 md:px-8">
        <fetcher.Form method="post">
          <Card className="h-full">
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-3">
              <CardTitle className="text-base">Beatled</CardTitle>
              <Button
                type="button"
                variant="ghost"
                size="icon"
                className="h-8 w-8 text-muted-foreground"
                aria-label="Refresh status"
                onClick={() => fetcher.submit(null)}
              >
                <RefreshCw className="h-4 w-4" />
              </Button>
            </CardHeader>
            <CardContent>
              {data.error ? (
                <p className="text-sm text-muted-foreground">{data.status as string}</p>
              ) : (
                <Table>
                  <TableBody>
                    <TableRow>
                      <TableCell className="font-medium">Last update</TableCell>
                      <TableCell className="text-right">
                        {data.x && format(data.x, "h:mm:ss a")}
                      </TableCell>
                    </TableRow>
                    <TableRow>
                      <TableCell className="font-medium">Tempo</TableCell>
                      <TableCell className="text-right">
                        {data.tempo && (
                          <FormattedNumber
                            value={data.tempo}
                            minimumFractionDigits={1}
                            maximumFractionDigits={2}
                          />
                        )}
                      </TableCell>
                    </TableRow>
                    <TableRow>
                      <TableCell className="font-medium">Uptime</TableCell>
                      <TableCell className="text-right">{serverUptime ?? "—"}</TableCell>
                    </TableRow>
                    <TableRow>
                      <TableCell className="font-medium">Devices</TableCell>
                      <TableCell className="text-right">{data.deviceCount ?? 0}</TableCell>
                    </TableRow>
                    {data.status &&
                      typeof data.status === "object" &&
                      Object.entries(data.status).map(([controllerId, controllerStatus]) => (
                        <TableRow key={controllerId}>
                          <TableCell className="font-medium">{controllerId}</TableCell>
                          <TableCell className="text-right">
                            <Switch
                              checked={controllerStatus as boolean}
                              onCheckedChange={(checked) => toggleService(controllerId, checked)}
                              aria-label={`Toggle ${controllerId}`}
                            />
                          </TableCell>
                        </TableRow>
                      ))}
                  </TableBody>
                </Table>
              )}
            </CardContent>
            <CardFooter>
              <p className="flex items-center gap-1 text-xs text-muted-foreground">
                <Clock className="h-3 w-3" />
                {data.x && "Updated " + formatDistanceToNow(data.x, { addSuffix: true })}
              </p>
            </CardFooter>
          </Card>
        </fetcher.Form>

        <FleetQosCard qos={qos} />

        <Card className="md:col-span-2">
          <CardHeader className="pb-3">
            <CardTitle className="text-base">Devices</CardTitle>
          </CardHeader>
          <CardContent>
            {devices.length === 0 ? (
              <p className="text-sm text-muted-foreground">No devices connected</p>
            ) : (
              <Table>
                <TableHeader>
                  <TableRow>
                    <TableHead>Board ID</TableHead>
                    <TableHead>IP Address</TableHead>
                    <TableHead>Firmware</TableHead>
                    <TableHead className="text-right">Offset</TableHead>
                    <TableHead className="text-right">RTT</TableHead>
                    <TableHead className="text-right">NB gaps</TableHead>
                    <TableHead className="text-right">Uptime</TableHead>
                    <TableHead className="text-right">Last Seen</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {devices.map((device) => (
                    <TableRow key={device.client_id}>
                      <TableCell className="font-mono text-xs">{device.board_id}</TableCell>
                      <TableCell>{device.ip_address}</TableCell>
                      <TableCell className="font-mono text-xs">
                        {formatFirmware(device)}
                      </TableCell>
                      <TableCell className="text-right font-mono text-xs">
                        {formatOffset(device)}
                      </TableCell>
                      <TableCell className="text-right font-mono text-xs">
                        {formatRtt(device)}
                      </TableCell>
                      <TableCell className="text-right font-mono text-xs">
                        {formatLoss(device)}
                      </TableCell>
                      <TableCell className="text-right font-mono text-xs">
                        {formatDeviceUptime(device)}
                      </TableCell>
                      <TableCell className="text-right">
                        {formatLastSeen(device.last_status_time)}
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            )}
          </CardContent>
        </Card>
      </div>
    </>
  );
}
