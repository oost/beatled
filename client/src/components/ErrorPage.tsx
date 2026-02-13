import { useRouteError, isRouteErrorResponse } from "react-router-dom";

export default function ErrorPage() {
  const error = useRouteError();
  console.error(error);

  const message = isRouteErrorResponse(error)
    ? error.statusText
    : error instanceof Error
      ? error.message
      : "Unknown error";

  return (
    <div className="flex min-h-[50vh] w-full flex-col items-center justify-center">
      <h1 className="text-2xl font-bold">Oops!</h1>
      <p className="mt-2 text-muted-foreground">Sorry, an unexpected error has occurred.</p>
      <p className="mt-1 italic text-muted-foreground">{message}</p>
    </div>
  );
}
