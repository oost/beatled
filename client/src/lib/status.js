import { getEndpoint, postEndpoint } from "./api";

export async function getStatus() {
  try {
    const res = await getEndpoint("/api/status");
    return res.json();
  } catch (err) {
    return { error: true, status: "Network error" };
  }
}

export async function serviceControl(serviceId, status) {
  try {
    const res = await postEndpoint("/api/service/control", {
      id: serviceId,
      status,
    });
    return res.json();
  } catch (err) {
    return { error: true, status: "Network error" };
  }
}
