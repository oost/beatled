let API_HOST = window.location.origin;

export function setAPIHost(new_host: string) {
  console.log(`Changed API_HOST to ${new_host}`);
  API_HOST = new_host;
}

export function getAPIHost(): string {
  return API_HOST;
}

const API_TOKEN_KEY = "beatled_api_token";

export function setAPIToken(token: string) {
  if (token) {
    sessionStorage.setItem(API_TOKEN_KEY, token);
  } else {
    sessionStorage.removeItem(API_TOKEN_KEY);
  }
}

export function getAPIToken(): string {
  return sessionStorage.getItem(API_TOKEN_KEY) ?? "";
}

type HttpMethod = "GET" | "POST" | "PUT" | "DELETE";

async function executeFetch(
  endpoint: string,
  method: HttpMethod,
  body?: unknown,
): Promise<Response> {
  try {
    const response = await fetch(new URL(endpoint, API_HOST), {
      method,
      mode: "cors",
      cache: "no-cache",
      credentials: "same-origin",
      headers: {
        "Content-Type": "application/json",
        ...(getAPIToken() && { Authorization: `Bearer ${getAPIToken()}` }),
      },
      body: body ? JSON.stringify(body) : undefined,
      redirect: "follow",
      referrerPolicy: "no-referrer",
    });
    if (!response.ok) {
      console.error(`fetch error ${response.status}: ${response.statusText}`);
      throw Error(response.statusText);
    }
    return response;
  } catch (err) {
    console.error(`Error calling ${method} ${endpoint}: ${err}`);
    throw err;
  }
}

export function getEndpoint(endpoint: string): Promise<Response> {
  return executeFetch(endpoint, "GET");
}

export function postEndpoint(endpoint: string, body?: unknown): Promise<Response> {
  return executeFetch(endpoint, "POST", body);
}
