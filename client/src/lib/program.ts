import { getEndpoint, postEndpoint, toApiFailure, invalidResponse, type ApiErrorKind } from "./api";

export interface ProgramInfo {
  error?: boolean;
  kind?: ApiErrorKind;
  httpStatus?: number;
  status?: string;
  programId?: number;
  programs?: { id: number; name: string }[];
}

export function isProgramInfo(v: unknown): v is ProgramInfo {
  if (typeof v !== "object" || v === null || Array.isArray(v)) return false;
  const o = v as Record<string, unknown>;
  if (o.programId !== undefined && typeof o.programId !== "number") return false;
  if (o.programs !== undefined) {
    if (!Array.isArray(o.programs)) return false;
    const valid = o.programs.every(
      (p: unknown) =>
        typeof p === "object" &&
        p !== null &&
        typeof (p as Record<string, unknown>).id === "number" &&
        typeof (p as Record<string, unknown>).name === "string",
    );
    if (!valid) return false;
  }
  return true;
}

export async function getProgram(): Promise<ProgramInfo> {
  try {
    const res = await getEndpoint("/api/program");
    const json: unknown = await res.json();
    return isProgramInfo(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}

export async function postProgram(programId: number): Promise<ProgramInfo> {
  try {
    const res = await postEndpoint("/api/program", {
      programId,
    });
    const json: unknown = await res.json();
    return isProgramInfo(json) ? json : invalidResponse();
  } catch (err) {
    return toApiFailure(err);
  }
}
