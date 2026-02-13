import { useRouteError, isRouteErrorResponse } from "react-router-dom";

export function RootErrorBoundary() {
  const error = useRouteError();

  let errorMessage = <div>Something went wrong</div>;
  if (isRouteErrorResponse(error)) {
    if (error.status === 404) {
      errorMessage = <div>This page doesn&apos;t exist!</div>;
    }

    if (error.status === 401) {
      errorMessage = <div>You aren&apos;t authorized to see this</div>;
    }

    if (error.status === 503) {
      errorMessage = <div>Looks like our API is down</div>;
    }

    if (error.status === 418) {
      errorMessage = <div>🫖</div>;
    }
  }

  return (
    <div className="flex min-h-screen w-full items-center justify-center">
      {errorMessage}
    </div>
  );
}
