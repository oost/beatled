import { getEndpoint, postEndpoint } from "./api";

export async function getLogs() {
  try {
    const res = await getEndpoint("/api/log");
    return res.json();
  } catch (err) {
    return { error: true, status: "Network error" };
  }
}
