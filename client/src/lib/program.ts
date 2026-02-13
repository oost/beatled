import { getEndpoint, postEndpoint } from "./api";

export interface ProgramInfo {
  error?: boolean;
  status?: string;
  programId?: number;
  programs?: { id: number; name: string }[];
}

export async function getProgram(): Promise<ProgramInfo> {
  try {
    const res = await getEndpoint("/api/program");
    return res.json();
  } catch (_err) {
    return { error: true, status: "Network error" };
  }
}

export async function postProgram(programId: number): Promise<ProgramInfo> {
  try {
    const res = await postEndpoint("/api/program", {
      programId,
    });
    return res.json();
  } catch (_err) {
    return { error: true, status: "Network error" };
  }
}
