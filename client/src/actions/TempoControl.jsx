import React, { useState, useEffect } from "react";
import Button from "react-bootstrap/Button";

export function TempoControl() {
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [status, setStatus] = useState(null);

  // Note: the empty deps array [] means
  // this useEffect will run once
  // similar to componentDidMount()

  const getTempo = () => {
    setIsLoading(true);
    fetch("/api/tempo", {
      method: "GET", // *GET, POST, PUT, DELETE, etc.
      mode: "cors", // no-cors, *cors, same-origin
      cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
      credentials: "same-origin", // include, *same-origin, omit
      headers: {
        "Content-Type": "application/json",
        // 'Content-Type': 'application/x-www-form-urlencoded',
      },
      redirect: "follow", // manual, *follow, error
      referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
    })
      .then((res) => res.json())
      .then(
        (result) => {
          setStatus(result);
        },
        // Note: it's important to handle errors here
        // instead of a catch() block so that we don't swallow
        // exceptions from actual bugs in components.
        (error) => {
          setError(error);
        }
      )
      .then(() => setIsLoading(false));
  };

  useEffect(() => {
    const timer = setTimeout(() => {
      getTempo();
    }, 2000);

    return () => clearTimeout(timer);
  });
  let message = "";
  if (error) {
    message = <div>Error: {error.message}</div>;
  } else if (isLoading) {
    message = <div>Loading...</div>;
  } else if (status) {
    message = (
      <p>
        Tempo: {status.tempo}. Time ref: {status.time_ref}
      </p>
    );
  }
  return (
    <div>
      {message}
      <Button variant="primary" onClick={() => getTempo()}>
        Get Tempo
      </Button>{" "}
    </div>
  );
}
