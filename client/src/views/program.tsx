import { useFetcher } from "react-router-dom";
import { useEffect, useState } from "react";
import { useInterval } from "../hooks/interval";
import PageHeader from "../components/page-header";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { Switch } from "@/components/ui/switch";
import { Button } from "@/components/ui/button";
import { FormattedNumber } from "react-intl";
import BeatChart from "../components/BeatChart";
import { getProgram, postProgram, type ProgramInfo } from "../lib/program";
import { getStatus, serviceControl, setManualBpm, type StatusResponse } from "../lib/status";

const BEAT_DETECTOR_ID = "beat-detector";
const MANUAL_BPM_ID = "manual-bpm";

interface TempoHistoryEntry {
  x: Date;
  y: number;
}

// Module-level ring buffer so the beat-history chart survives re-renders and
// accumulates across poll ticks, mirroring the Status page's old behaviour.
const tempoHistory: TempoHistoryEntry[] = [];
const MAX_HISTORY = 30;

export async function loader() {
  const [program, status] = await Promise.all([getProgram(), getStatus()]);
  if (!status.error && status.tempo) {
    tempoHistory.push({ x: new Date(), y: status.tempo });
    if (tempoHistory.length > MAX_HISTORY) {
      tempoHistory.splice(0, tempoHistory.length - MAX_HISTORY);
    }
  }
  return { program, status, history: [...tempoHistory] };
}

export async function action() {
  return true;
}

interface LoaderData {
  program: ProgramInfo;
  status: StatusResponse;
  history: TempoHistoryEntry[];
}

function serviceRunning(status: StatusResponse, id: string): boolean {
  return (
    !!status.status &&
    typeof status.status === "object" &&
    (status.status as Record<string, boolean>)[id] === true
  );
}

export default function ProgramPage() {
  const fetcher = useFetcher();

  // Poll so the tempo chart, detected tempo, and service toggles stay live.
  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit(null);
    }
  }, 2 * 1000);

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit(null);
    }
  }, [fetcher]);

  const data = fetcher.data as LoaderData | undefined;
  const program = data?.program;
  const status = data?.status ?? {};
  const history = data?.history ?? [];

  const detectorOn = serviceRunning(status, BEAT_DETECTOR_ID);
  const manualOn = serviceRunning(status, MANUAL_BPM_ID);

  // Local, editable copy of the manual BPM. Seeded from the server and kept in
  // sync unless the user is mid-edit (focused), so polling doesn't clobber
  // their typing.
  const [bpmInput, setBpmInput] = useState<string>("");
  const [bpmFocused, setBpmFocused] = useState(false);
  useEffect(() => {
    if (!bpmFocused && status.manualBpm !== undefined) {
      setBpmInput(String(Math.round(status.manualBpm)));
    }
  }, [status.manualBpm, bpmFocused]);

  const refresh = () => fetcher.submit(null);

  const onSelectProgram = async (value: string) => {
    await postProgram(parseInt(value));
    refresh();
  };

  const onToggleService = async (id: string, checked: boolean) => {
    await serviceControl(id, checked);
    refresh();
  };

  const onApplyBpm = async () => {
    const bpm = parseFloat(bpmInput);
    if (!Number.isFinite(bpm)) return;
    const clamped = Math.min(400, Math.max(20, bpm));
    await setManualBpm(clamped);
    setBpmInput(String(Math.round(clamped)));
    refresh();
  };

  return (
    <>
      <PageHeader title="Program" />
      <div className="grid gap-4 px-4 pb-8 md:grid-cols-2 md:px-8">
        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-base">LED Program</CardTitle>
          </CardHeader>
          <CardContent>
            {program?.error ? (
              <p className="text-sm text-muted-foreground">{program.status}</p>
            ) : (
              <RadioGroup
                value={program ? String(program.programId) : undefined}
                onValueChange={onSelectProgram}
              >
                {program?.programs?.map((p) => (
                  <div key={p.id} className="flex items-center space-x-3 py-2">
                    <RadioGroupItem value={String(p.id)} id={`program-${p.id}`} />
                    <Label htmlFor={`program-${p.id}`} className="text-sm font-normal">
                      {p.name}
                    </Label>
                  </div>
                ))}
              </RadioGroup>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-base">Tempo</CardTitle>
          </CardHeader>
          <CardContent className="space-y-5">
            <div className="flex items-center justify-between">
              <div>
                <Label className="text-sm font-medium">Beat detector</Label>
                <p className="text-xs text-muted-foreground">Track tempo from audio input</p>
              </div>
              <Switch
                checked={detectorOn}
                onCheckedChange={(c) => onToggleService(BEAT_DETECTOR_ID, c)}
                aria-label="Toggle beat detector"
              />
            </div>

            <div className="flex items-center justify-between">
              <div>
                <Label className="text-sm font-medium">Manual BPM</Label>
                <p className="text-xs text-muted-foreground">
                  Drive a fixed tempo (turns off the beat detector)
                </p>
              </div>
              <Switch
                checked={manualOn}
                onCheckedChange={(c) => onToggleService(MANUAL_BPM_ID, c)}
                aria-label="Toggle manual BPM"
              />
            </div>

            <div className="flex items-end gap-2">
              <div className="flex-1">
                <Label htmlFor="manual-bpm-input" className="text-xs text-muted-foreground">
                  BPM (20–400)
                </Label>
                <input
                  id="manual-bpm-input"
                  type="number"
                  min={20}
                  max={400}
                  step={1}
                  value={bpmInput}
                  disabled={!manualOn}
                  onFocus={() => setBpmFocused(true)}
                  onBlur={() => setBpmFocused(false)}
                  onChange={(e) => setBpmInput(e.target.value)}
                  onKeyDown={(e) => {
                    if (e.key === "Enter") {
                      e.preventDefault();
                      void onApplyBpm();
                    }
                  }}
                  className="mt-1 flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring disabled:cursor-not-allowed disabled:opacity-50"
                />
              </div>
              <Button type="button" onClick={() => void onApplyBpm()} disabled={!manualOn}>
                Set
              </Button>
            </div>

            <div className="flex items-center justify-between border-t pt-3 text-sm">
              <span className="text-muted-foreground">Current tempo</span>
              <span className="font-medium">
                {status.tempo ? (
                  <FormattedNumber
                    value={status.tempo}
                    minimumFractionDigits={1}
                    maximumFractionDigits={1}
                  />
                ) : (
                  "—"
                )}{" "}
                BPM
              </span>
            </div>
          </CardContent>
        </Card>

        <Card className="md:col-span-2">
          <CardHeader className="pb-3">
            <CardTitle className="text-base">Beat History</CardTitle>
          </CardHeader>
          <CardContent>
            <BeatChart historyData={history} />
          </CardContent>
        </Card>
      </div>
    </>
  );
}
