import { Navigate } from "react-router-dom";

export default function Index() {
  return (
    <p id="zero-state">
      Welcome to Beatled.
      <br />
      Check out{" "}
      <a href="https://github.com/oost/rpiz-beatserver">GitHub repo.</a>.
      <Navigate to="/status" replace={true} />
    </p>
  );
}
