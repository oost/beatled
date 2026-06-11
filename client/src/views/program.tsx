import { useFetcher } from "react-router-dom";
import { useEffect, useRef, useState } from "react";
import { useInterval } from "../hooks/interval";
import PageHeader from "../components/page-header";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { RadioGroup, RadioGroupCard } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { Slider } from "@/components/ui/slider";
import { FormattedNumber } from "react-intl";
import BeatChart, { WINDOW_MS } from "../components/BeatChart";
import { getProgram, postProgram, type ProgramInfo } from "../lib/program";
import {
  getStatus,
  serviceControl,
  setManualBpm,
  type StatusResponse,
} from "../lib/status";

const BEAT_DETECTOR_ID = "beat-detector";
const MANUAL_BPM_ID = "manual-bpm";

// Slider bounds for the manual tempo. The server accepts 20-400, but
// practically all music sits in this range and the narrower span makes
// individual BPM values reachable by drag.
const MIN_BPM = 60;
const MAX_BPM = 200;

interface TempoHistoryEntry {
  x: Date;
  y: number;
}

// Module-level ring buffer so the beat-history chart survives re-renders and
// accumulates across poll ticks, mirroring the Status page's old behaviour.
const tempoHistory: TempoHistoryEntry[] = [];
// Must comfortably cover the chart's sliding window (WINDOW_MS at one point
// per 2s poll), so the line reaches the left edge.
const MAX_HISTORY = (WINDOW_MS / 1000) * 1.5;

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

type TempoMode = "beat" | "manual";

export default function ProgramPage() {
  const fetcher = useFetcher();

  // Poll so the tempo chart, detected tempo, and mode selector stay live.
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

  // Strict two-mode model: one tempo source is always presented as active.
  // If the server reports both services off (e.g. right after boot) we render
  // Beat tracking without POSTing anything until the user interacts.
  const mode: TempoMode = serviceRunning(status, MANUAL_BPM_ID) ? "manual" : "beat";

  // Local, editable copy of the manual BPM. Seeded from the server and kept
  // in sync unless the user is mid-drag, so polling doesn't yank the thumb.
  const [bpm, setBpm] = useState(120);
  const sliderActive = useRef(false);
  useEffect(() => {
    if (!sliderActive.current && status.manualBpm !== undefined) {
      setBpm(Math.min(MAX_BPM, Math.max(MIN_BPM, Math.round(status.manualBpm))));
    }
  }, [status.manualBpm]);

  const refresh = () => fetcher.submit(null);

  const onSelectProgram = async (value: string) => {
    await postProgram(parseInt(value));
    refresh();
  };

  const onSelectMode = async (value: string) => {
    const target = value === "manual" ? MANUAL_BPM_ID : BEAT_DETECTOR_ID;
    const other = value === "manual" ? BEAT_DETECTOR_ID : MANUAL_BPM_ID;
    // The server stops the other tempo source when one starts; the explicit
    // second call keeps the strict two-mode contract client-side too.
    await serviceControl(target, true);
    await serviceControl(other, false);
    refresh();
  };

  const onCommitBpm = async (value: number) => {
    sliderActive.current = false;
    await setManualBpm(value);
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
                value={program ? String(program.programId) : ""}
                onValueChange={onSelectProgram}
                className="grid grid-cols-2 gap-3 sm:grid-cols-3"
              >
                {program?.programs?.map((p) => (
                  <RadioGroupCard
                    key={p.id}
                    value={String(p.id)}
                    className="flex aspect-square items-center justify-center p-2 text-center"
                  >
                    {p.name}
                  </RadioGroupCard>
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
            <RadioGroup
              value={mode}
              onValueChange={onSelectMode}
              className="grid grid-cols-2 gap-2"
              aria-label="Tempo source"
            >
              <RadioGroupCard value="beat" className="px-3 py-2.5">
                Beat tracking
              </RadioGroupCard>
              <RadioGroupCard value="manual" className="px-3 py-2.5">
                Manual
              </RadioGroupCard>
            </RadioGroup>

            {mode === "manual" && (
              <div className="space-y-2">
                <div className="flex items-center justify-between">
                  <Label htmlFor="manual-bpm-slider" className="text-xs text-muted-foreground">
                    BPM ({MIN_BPM}–{MAX_BPM})
                  </Label>
                  <span className="text-sm font-medium tabular-nums">{bpm}</span>
                </div>
                <Slider
                  id="manual-bpm-slider"
                  aria-label="Manual BPM"
                  min={MIN_BPM}
                  max={MAX_BPM}
                  step={1}
                  value={[bpm]}
                  onValueChange={([v]) => {
                    sliderActive.current = true;
                    setBpm(v);
                  }}
                  onValueCommit={([v]) => void onCommitBpm(v)}
                />
              </div>
            )}

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
