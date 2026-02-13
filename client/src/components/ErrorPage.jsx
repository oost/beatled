import { useRouteError } from "react-router-dom";

export default function ErrorPage() {
  const error = useRouteError();
  console.error(error);

  return (
    <div className="flex min-h-[50vh] w-full flex-col items-center justify-center">
      <h1 className="text-2xl font-bold">Oops!</h1>
      <p className="mt-2 text-muted-foreground">
        Sorry, an unexpected error has occurred.
      </p>
      <p className="mt-1 italic text-muted-foreground">
        {error.statusText || error.message}
      </p>
    </div>
  );
}
