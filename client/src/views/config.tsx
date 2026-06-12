import { useCallback, useEffect, useRef, useState } from "react";
import { useLoaderData, useSubmit } from "react-router-dom";
import type { ActionFunctionArgs } from "react-router-dom";
import PageHeader from "../components/page-header";
import { Card, CardContent } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { Button } from "@/components/ui/button";
import {
  getAPIHost,
  setAPIHost,
  getAPIToken,
  setAPIToken,
  pingHealth,
} from "../lib/api";
import { apControl } from "../lib/ap";
import { useInterval } from "../hooks/interval";

export async function loader() {
  return { host: getAPIHost(), token: getAPIToken() };
}

export async function action({ request }: ActionFunctionArgs) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  if (updates.host) {
    // setAPIHost validates the URL and returns false (logging why) on
    // anything that isn't absolute http(s) — the stored host is then
    // left untouched.
    setAPIHost(updates.host as string);
  }
  if (formData.has("token")) {
    setAPIToken(updates.token as string);
  }
  return true;
}

const API_HOSTS = [
  { name: "beatled.local", value: "https://beatled.local:8443" },
  { name: "beatled.test", value: "https://beatled.test:8443" },
  { name: "Raspberry Pi", value: "https://raspberrypi1.local:8443" },
  { name: "Localhost", value: "https://localhost:8443" },
  { name: "Vite Proxy", value: "http://localhost:5173" },
];

type HealthStatus = "unknown" | "ok" | "error";

function StatusDot({ status }: { status: HealthStatus }) {
  const color =
    status === "ok"
      ? "bg-green-500"
      : status === "error"
        ? "bg-red-500"
        : "bg-gray-400";
  return (
    <span className="relative inline-flex h-2.5 w-2.5">
      {status === "ok" && (
        <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-green-400 opacity-75" />
      )}
      <span className={`relative inline-flex h-2.5 w-2.5 rounded-full ${color}`} />
    </span>
  );
}

function useHealthChecks() {
  const [statuses, setStatuses] = useState<Record<string, HealthStatus>>(() =>
    Object.fromEntries(API_HOSTS.map((h) => [h.value, "unknown"])),
  );

  // A single AbortController per mounted hook instance so unmounting
  // immediately aborts every in-flight probe. Previously each call
  // spawned its own controller with no shared cleanup — a fast tab
  // switch could leave several open sockets per host until they timed
  // out.
  const abortRef = useRef<AbortController | null>(null);

  const checkAll = useCallback(() => {
    abortRef.current?.abort();
    abortRef.current = new AbortController();
    const signal = abortRef.current.signal;

    for (const host of API_HOSTS) {
      pingHealth(host.value, { signal })
        .then((ok) => {
          if (signal.aborted) return;
          setStatuses((prev) => ({
            ...prev,
            [host.value]: ok ? "ok" : "error",
          }));
        });
    }
  }, []);

  useEffect(() => {
    checkAll();
    return () => abortRef.current?.abort();
  }, [checkAll]);

  useInterval(checkAll, 5000);

  return statuses;
}

function AccessPointCard() {
  const [apStatus, setApStatus] = useState<"on" | "off" | "unknown">("unknown");
  const [revertMinutes, setRevertMinutes] = useState(10);
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState<string | null>(null);

  const refresh = useCallback(async () => {
    const res = await apControl("status");
    setApStatus(res.error ? "unknown" : (res.ap ?? "unknown"));
  }, []);

  useEffect(() => {
    void refresh();
  }, [refresh]);

  const switchOn = async () => {
    const revertNote =
      revertMinutes > 0 ? ` It will auto-revert to WiFi after ${revertMinutes} min.` : "";
    if (
      !window.confirm(
        `Switch the Pi to hotspot mode? This device will lose its connection to the server.${revertNote}\n\n` +
          `Rejoin the "Beatled" WiFi network, then open https://192.168.4.1:8443/ to continue.`,
      )
    ) {
      return;
    }
    setBusy(true);
    setMessage(
      'Switching to hotspot — this device will lose connection. Rejoin the "Beatled" WiFi and open https://192.168.4.1:8443/.',
    );
    // The response usually never returns (the Pi's radio switches mid-request),
    // so we don't depend on it; the message above stays up.
    await apControl("on", revertMinutes);
    setBusy(false);
  };

  const switchOff = async () => {
    setBusy(true);
    setMessage(null);
    const res = await apControl("off");
    setBusy(false);
    if (res.error) {
      setMessage(`Failed to switch to WiFi: ${res.status ?? "error"}`);
    } else {
      await refresh();
    }
  };

  return (
    <Card>
      <CardContent className="pt-6 space-y-4">
        <div className="flex items-center justify-between">
          <Label className="text-sm font-medium">Access Point</Label>
          <span className="text-sm text-muted-foreground">
            Hotspot is {apStatus === "unknown" ? "—" : apStatus}
          </span>
        </div>
        <p className="text-xs text-muted-foreground">
          Switch the Raspberry Pi between your WiFi and its own “Beatled” hotspot
          (192.168.4.1). The Pi has a single radio, so turning the hotspot on drops
          this connection — rejoin the “Beatled” network to reconnect.
        </p>
        <div className="flex flex-wrap items-end gap-3">
          <div className="flex flex-col gap-1">
            <Label
              htmlFor="ap-revert"
              className="text-xs font-normal text-muted-foreground"
            >
              Auto-revert (min)
            </Label>
            <input
              id="ap-revert"
              type="number"
              min={0}
              max={1440}
              value={revertMinutes}
              onChange={(e) => setRevertMinutes(Number(e.target.value))}
              className="flex h-9 w-24 rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
            />
          </div>
          <Button variant="outline" disabled={busy} onClick={switchOn}>
            Switch to Hotspot
          </Button>
          <Button variant="secondary" disabled={busy} onClick={switchOff}>
            Switch to WiFi
          </Button>
        </div>
        {message && <p className="text-xs text-amber-600">{message}</p>}
      </CardContent>
    </Card>
  );
}

export default function ConfigPage() {
  const submit = useSubmit();
  const { host, token } = useLoaderData() as { host: string; token: string };
  const healthStatuses = useHealthChecks();

  const onHostChange = useCallback((value: string) => {
    const formData = new FormData();
    formData.set("host", value);
    submit(formData, { method: "post" });
  }, [submit]);

  const didAutoSelect = useRef(false);
  useEffect(() => {
    if (didAutoSelect.current) return;
    const currentStatus = healthStatuses[host] as HealthStatus | undefined;
    if (currentStatus === "ok") {
      didAutoSelect.current = true;
      return;
    }
    if (currentStatus === "error" || !API_HOSTS.some((h) => h.value === host)) {
      const first = API_HOSTS.find((h) => healthStatuses[h.value] === "ok");
      if (first) {
        didAutoSelect.current = true;
        onHostChange(first.value);
      }
    }
  }, [healthStatuses, host, onHostChange]);

  const onTokenBlur = (e: React.FocusEvent<HTMLInputElement>) => {
    const formData = new FormData();
    formData.set("token", e.target.value);
    submit(formData, { method: "post" });
  };

  return (
    <>
      <PageHeader title="Config" />
      <div className="px-4 pb-8 md:px-8 space-y-4">
        <Card>
          <CardContent className="pt-6">
            <fieldset>
              <legend className="mb-3 text-sm font-medium">API Host</legend>
              <RadioGroup defaultValue={host} onValueChange={onHostChange}>
                {API_HOSTS.map((api_host) => (
                  <div key={api_host.value} className="flex items-center space-x-3 py-2">
                    <RadioGroupItem value={api_host.value} id={`host-${api_host.value}`} />
                    <Label htmlFor={`host-${api_host.value}`} className="text-sm font-normal flex items-center gap-2">
                      {api_host.name}{" "}
                      <span className="text-muted-foreground">
                        {new URL(api_host.value).hostname}
                      </span>
                      <StatusDot status={healthStatuses[api_host.value]} />
                    </Label>
                  </div>
                ))}
              </RadioGroup>
            </fieldset>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="pt-6">
            <Label htmlFor="api-token" className="mb-3 text-sm font-medium">
              API Token
            </Label>
            <input
              id="api-token"
              type="password"
              defaultValue={token}
              onBlur={onTokenBlur}
              placeholder="Leave empty if auth is disabled"
              className="mt-2 flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm transition-colors placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
            />
          </CardContent>
        </Card>
        <AccessPointCard />
      </div>
    </>
  );
}
