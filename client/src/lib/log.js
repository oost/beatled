import { getEndpoint, postEndpoint } from "./api";

export async function getLogs() {
  const res = await getEndpoint("/api/log");
  return res.json();
}
