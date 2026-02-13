import { Navigate } from "react-router-dom";

export default function Index() {
  return (
    <div className="flex items-center justify-center p-8 text-center text-muted-foreground">
      <p>
        Welcome to Beatled.
        <br />
        Check out{" "}
        <a
          href="https://github.com/oost/rpiz-beatserver"
          className="text-primary hover:underline"
        >
          GitHub repo.
        </a>
      </p>
      <Navigate to="/status" replace={true} />
    </div>
  );
}
