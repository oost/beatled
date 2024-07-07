let API_HOST = `https://${window.location.hostname}:8080`;

export function setAPIHost(new_host) {
  console.log(`Changed API_HOST to ${new_host}`);
  API_HOST = new_host;
}

export function getAPIHost() {
  return API_HOST;
}

async function executeFetch(endpoint, method, body) {
  try {
    const response = await fetch(new URL(endpoint, API_HOST), {
      method, // *GET, POST, PUT, DELETE, etc.
      mode: "cors", // no-cors, *cors, same-origin
      cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
      credentials: "same-origin", // include, *same-origin, omit
      headers: {
        "Content-Type": "application/json",
      },
      body: body && JSON.stringify(body),
      redirect: "follow", // manual, *follow, error
      referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
    });
    if (!response.ok) {
      console.log(`fetch error ${response.statusCode}: ${response.statusText}`);
      throw Error(response.statusText);
    }
    return response;
  } catch (err) {
    console.error(`Error calling ${method} ${endpoint}: ${err}`);
    throw err;
  }
}

export function getEndpoint(endpoint) {
  return executeFetch(endpoint, "GET");
}

export function postEndpoint(endpoint, body) {
  return executeFetch(endpoint, "POST", body);
}
