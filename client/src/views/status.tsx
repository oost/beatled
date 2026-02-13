import { useFetcher } from "react-router-dom";
import { useEffect } from "react";
import { useInterval } from "../hooks/interval";
import { getStatus, serviceControl } from "../lib/status";
import PageHeader from "../components/page-header";
import { Card, CardContent, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import { Table, TableBody, TableCell, TableRow } from "@/components/ui/table";
import { Switch } from "@/components/ui/switch";
import { Button } from "@/components/ui/button";
import { FormattedNumber } from "react-intl";
import BeatChart from "../components/BeatChart";
import { format, formatDistanceToNow } from "date-fns";
import { RefreshCw, Clock } from "lucide-react";

interface TempoHistoryEntry {
  x: Date;
  y: number;
  error?: boolean;
  status?: string | Record<string, boolean>;
  tempo?: number;
  deviceCount?: number;
}

const tempoHistory: TempoHistoryEntry[] = [];
const MAX_HISTORY = 30;

export async function loader() {
  console.log("Loading status");
  const status = await getStatus();
  const last: TempoHistoryEntry = { x: new Date(), y: status.tempo || NaN, ...status };
  tempoHistory.push(last);
  if (tempoHistory.length > MAX_HISTORY) {
    tempoHistory.splice(0, tempoHistory.length - MAX_HISTORY);
  }
  return {
    last,
    history: tempoHistory,
  };
}

export async function action() {
  return true;
}

export default function StatusPage() {
  const fetcher = useFetcher();

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

  const data =
    (fetcher.data as { last: TempoHistoryEntry; history: TempoHistoryEntry[] } | undefined)?.last ||
    ({} as Partial<TempoHistoryEntry>);
  const historyData = (fetcher.data as { history: TempoHistoryEntry[] } | undefined)?.history || [];

  return (
    <>
      <PageHeader title="Status" />
      <div className="space-y-4 px-4 pb-8 md:px-8">
        <fetcher.Form method="post">
          <Card>
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
                      <TableCell className="font-medium">Devices</TableCell>
                      <TableCell className="text-right">
                        {data.deviceCount ?? 0}
                      </TableCell>
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

        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-base">Beat History</CardTitle>
          </CardHeader>
          <CardContent>
            {data.error ? (
              <p className="text-sm text-muted-foreground">{data.status as string}</p>
            ) : (
              <BeatChart historyData={historyData} />
            )}
          </CardContent>
          <CardFooter>
            <p className="flex items-center gap-1 text-xs text-muted-foreground">
              <Clock className="h-3 w-3" />
              {data.x && "Updated " + formatDistanceToNow(data.x, { addSuffix: true })}
            </p>
          </CardFooter>
        </Card>
      </div>
    </>
  );
}
