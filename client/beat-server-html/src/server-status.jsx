import React, { useState } from 'react';
import Button from 'react-bootstrap/Button';


export function ServerStatus() {
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [status, setStatus] = useState(null);

  // Note: the empty deps array [] means
  // this useEffect will run once
  // similar to componentDidMount()
  const updateStatus = () => {
    setIsLoading(true);
    fetch("/api/status")
      .then(res => res.json())
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
      .then(() =>setIsLoading(false));
      
  };

  let message = "";
  if (error) {
    message = <div>Error: {error.message}</div>;
  } else if (isLoading) {
    message = <div>Loading...</div>;
  } else if (status) {
    message = (
      <p>{status.message}</p>
    );
  }
  return <div>
    <Button variant="primary" onClick={() => updateStatus()}>Update Status</Button>{' '}
    {message}
  </div>


}
