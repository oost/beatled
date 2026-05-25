import { getLogs, type LogResponse } from "../lib/log";
import { useFetcher } from "react-router-dom";
import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { useInterval } from "../hooks/interval";
import { Button } from "@/components/ui/button";
import { RefreshCw, Pause, Play, ArrowDown } from "lucide-react";
import { getConsoleLogs } from "../lib/console";

const REFRESH_INTERVAL_MS = 3000;

export async function loader() {
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
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);
  const [, setNow] = useState(Date.now()); // ticks once a second to keep the "Xs ago" label live

  useInterval(
    () => {
      if (autoRefresh && fetcher.state === "idle" && document.visibilityState === "visible") {
        fetcher.submit(null);
      }
    },
    REFRESH_INTERVAL_MS,
  );

  useInterval(() => setNow(Date.now()), 1000);

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit(null);
    }
  }, [fetcher]);

  useEffect(() => {
    if (fetcher.data) {
      setLastUpdated(new Date());
    }
  }, [fetcher.data]);

  const data = fetcher.data as LogData | undefined;
  const logs = data?.logs || [];
  const consoleLogs = data?.consoleLogs || [];
  const isError = !Array.isArray(logs) && (logs as LogResponse).error;
  const serverLines = isError ? [] : (logs as string[]);

  return (
    <div className="fixed inset-x-0 top-0 bottom-16 flex flex-col bg-background safe-bottom">
      <header className="flex items-center justify-between gap-2 border-b border-border px-4 py-2">
        <div className="flex items-baseline gap-3">
          <h1 className="text-base font-semibold tracking-tight">Logs</h1>
          <span className="text-xs text-muted-foreground tabular-nums">
            {lastUpdated ? `updated ${formatRelative(lastUpdated)}` : "loading…"}
          </span>
        </div>
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="icon"
            type="button"
            className="h-8 w-8"
            aria-label={autoRefresh ? "Pause auto-refresh" : "Resume auto-refresh"}
            onClick={() => setAutoRefresh((v) => !v)}
          >
            {autoRefresh ? <Pause className="h-4 w-4" /> : <Play className="h-4 w-4" />}
          </Button>
          <Button
            variant="ghost"
            size="icon"
            type="button"
            className="h-8 w-8"
            aria-label="Refresh now"
            disabled={fetcher.state !== "idle"}
            onClick={() => fetcher.submit(null)}
          >
            <RefreshCw className={`h-4 w-4 ${fetcher.state !== "idle" ? "animate-spin" : ""}`} />
          </Button>
        </div>
      </header>

      <fetcher.Form method="post" className="hidden" />

      <div className="grid min-h-0 flex-1 grid-rows-2 gap-px bg-border md:grid-cols-2 md:grid-rows-1">
        <LogPane
          title="Server"
          lines={serverLines}
          errorMessage={isError ? (logs as LogResponse).status : undefined}
        />
        <LogPane title="Console" lines={consoleLogs} />
      </div>
    </div>
  );
}

function LogPane({
  title,
  lines,
  errorMessage,
}: {
  title: string;
  lines: string[];
  errorMessage?: string;
}) {
  const scrollRef = useRef<HTMLDivElement>(null);
  const [stickToBottom, setStickToBottom] = useState(true);

  useLayoutEffect(() => {
    const el = scrollRef.current;
    if (!el || !stickToBottom) return;
    el.scrollTop = el.scrollHeight;
  }, [lines, stickToBottom]);

  const handleScroll = () => {
    const el = scrollRef.current;
    if (!el) return;
    const atBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 24;
    setStickToBottom(atBottom);
  };

  const scrollToBottom = () => {
    const el = scrollRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
    setStickToBottom(true);
  };

  return (
    <section className="flex min-h-0 min-w-0 flex-col bg-background">
      <div className="flex items-center justify-between border-b border-border bg-muted/30 px-3 py-1.5">
        <h2 className="text-xs font-medium uppercase tracking-wide text-muted-foreground">
          {title}
          <span className="ml-2 text-muted-foreground/70 normal-case tracking-normal">
            {lines.length} lines
          </span>
        </h2>
        {!stickToBottom && (
          <Button
            variant="ghost"
            size="sm"
            type="button"
            className="h-6 gap-1 px-2 text-xs"
            onClick={scrollToBottom}
          >
            <ArrowDown className="h-3 w-3" />
            Jump to latest
          </Button>
        )}
      </div>
      <div ref={scrollRef} onScroll={handleScroll} className="flex-1 overflow-auto bg-muted/20">
        {errorMessage ? (
          <p className="p-3 text-sm text-muted-foreground">{errorMessage}</p>
        ) : (
          <pre className="m-0 whitespace-pre-wrap break-words p-3 font-mono text-xs leading-relaxed">
            {lines.length === 0 ? (
              <span className="text-muted-foreground">No log lines</span>
            ) : (
              lines.map((logLine, idx) => (
                <span key={`${title}-${idx}-${logLine.slice(0, 32)}`}>{logLine}</span>
              ))
            )}
          </pre>
        )}
      </div>
    </section>
  );
}

function formatRelative(date: Date): string {
  const secs = Math.max(0, Math.floor((Date.now() - date.getTime()) / 1000));
  if (secs < 5) return "just now";
  if (secs < 60) return `${secs}s ago`;
  const mins = Math.floor(secs / 60);
  return `${mins}m ago`;
}
