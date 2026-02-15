import { useCallback, useEffect, useState } from "react";
import { useLoaderData, useSubmit } from "react-router-dom";
import type { ActionFunctionArgs } from "react-router-dom";
import PageHeader from "../components/page-header";
import { Card, CardContent } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { getAPIHost, setAPIHost, getAPIToken, setAPIToken } from "../lib/api";
import { useInterval } from "../hooks/interval";

export async function loader() {
  return { host: getAPIHost(), token: getAPIToken() };
}

export async function action({ request }: ActionFunctionArgs) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  if (updates.host) {
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

  const checkAll = useCallback(() => {
    for (const host of API_HOSTS) {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 3000);

      fetch(new URL("/api/health", host.value), {
        signal: controller.signal,
        mode: "cors",
        cache: "no-cache",
      })
        .then((res) => {
          clearTimeout(timeout);
          setStatuses((prev) => ({
            ...prev,
            [host.value]: res.ok ? "ok" : "error",
          }));
        })
        .catch(() => {
          clearTimeout(timeout);
          setStatuses((prev) => ({ ...prev, [host.value]: "error" }));
        });
    }
  }, []);

  useEffect(() => {
    checkAll();
  }, [checkAll]);

  useInterval(checkAll, 5000);

  return statuses;
}

export default function ConfigPage() {
  const submit = useSubmit();
  const { host, token } = useLoaderData() as { host: string; token: string };
  const healthStatuses = useHealthChecks();

  const onHostChange = (value: string) => {
    const formData = new FormData();
    formData.set("host", value);
    submit(formData, { method: "post" });
  };

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
      </div>
    </>
  );
}
