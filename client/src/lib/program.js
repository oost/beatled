import { getEndpoint, postEndpoint } from "./api";

export async function getProgram() {
  try {
    const res = await getEndpoint("/api/program");
    return res.json();
  } catch (err) {
    return { error: true, status: "Network error" };
  }
}

export async function postProgram(programId) {
  try {
    const res = await postEndpoint("/api/program", {
      programId,
    });
    return res.json();
  } catch (err) {
    return { error: true, status: "Network error" };
  }
}
