import { getLogs, type LogResponse } from "../lib/log";
import { useFetcher } from "react-router-dom";
import { useEffect } from "react";
import { useInterval } from "../hooks/interval";
import PageHeader from "../components/page-header";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { RefreshCw } from "lucide-react";
import { getConsoleLogs } from "../lib/console";

export async function loader() {
  console.log("Loading status");
  return { logs: await getLogs(), consoleLogs: getConsoleLogs() };
}

export async function action() {
  return true;
}

interface LogData {
  logs: string[] | LogResponse;
  consoleLogs: string[];
}

export default function LogPage() {
  const fetcher = useFetcher();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit(null);
    }
  }, 10 * 1000);

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit(null);
    }
  }, [fetcher]);

  const data = fetcher.data as LogData | undefined;
  const logs = data?.logs || [];
  const consoleLogs = data?.consoleLogs || [];

  const isError = !Array.isArray(logs) && (logs as LogResponse).error;

  return (
    <>
      <PageHeader title="Logs" />
      <div className="space-y-4 px-4 pb-8 md:px-8">
        <fetcher.Form method="post">
          <Card>
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-3">
              <CardTitle className="text-base">Server</CardTitle>
              <Button
                variant="ghost"
                size="icon"
                type="button"
                className="h-8 w-8 text-muted-foreground"
                aria-label="Refresh server logs"
                onClick={() => fetcher.submit(null)}
              >
                <RefreshCw className="h-4 w-4" />
              </Button>
            </CardHeader>
            <CardContent>
              {isError ? (
                <p className="text-sm text-muted-foreground">{(logs as LogResponse).status}</p>
              ) : (
                <pre className="max-h-80 overflow-auto rounded-lg bg-muted/50 p-3 font-mono text-xs leading-relaxed">
                  {(logs as string[]).map((logLine: string, idx: number) => (
                    <span key={idx}>{logLine}</span>
                  ))}
                </pre>
              )}
            </CardContent>
          </Card>

          <Card className="mt-4">
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-3">
              <CardTitle className="text-base">Console</CardTitle>
              <Button
                variant="ghost"
                size="icon"
                type="button"
                className="h-8 w-8 text-muted-foreground"
                aria-label="Refresh console logs"
                onClick={() => fetcher.submit(null)}
              >
                <RefreshCw className="h-4 w-4" />
              </Button>
            </CardHeader>
            <CardContent>
              <pre className="max-h-80 overflow-auto rounded-lg bg-muted/50 p-3 font-mono text-xs leading-relaxed">
                {consoleLogs.map((logLine: string, idx: number) => (
                  <span key={idx}>{logLine}</span>
                ))}
              </pre>
            </CardContent>
          </Card>
        </fetcher.Form>
      </div>
    </>
  );
}
