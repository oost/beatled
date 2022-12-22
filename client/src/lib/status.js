import { getEndpoint, postEndpoint } from "./api";

export async function getStatus() {
  const res = await getEndpoint("/api/status");
  return res.json();
}

export async function serviceControl(serviceId, status) {
  const res = await postEndpoint("/api/service/control", {
    id: serviceId,
    status,
  });
  return res.json();
}
