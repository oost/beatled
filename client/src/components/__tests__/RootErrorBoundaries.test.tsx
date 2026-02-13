import { describe, it, expect, vi, type Mock } from "vitest";
import { render, screen } from "@testing-library/react";

vi.mock("react-router-dom", async () => {
  return {
    useRouteError: vi.fn(),
    isRouteErrorResponse: vi.fn(),
  };
});

import { useRouteError, isRouteErrorResponse } from "react-router-dom";
import { RootErrorBoundary } from "../RootErrorBoundaries";

describe("RootErrorBoundary", () => {
  it("shows 404 message", () => {
    (useRouteError as Mock).mockReturnValue({ status: 404 });
    (isRouteErrorResponse as unknown as Mock).mockReturnValue(true);

    render(<RootErrorBoundary />);
    expect(screen.getByText("This page doesn't exist!")).toBeInTheDocument();
  });

  it("shows 401 message", () => {
    (useRouteError as Mock).mockReturnValue({ status: 401 });
    (isRouteErrorResponse as unknown as Mock).mockReturnValue(true);

    render(<RootErrorBoundary />);
    expect(screen.getByText("You aren't authorized to see this")).toBeInTheDocument();
  });

  it("shows 503 message", () => {
    (useRouteError as Mock).mockReturnValue({ status: 503 });
    (isRouteErrorResponse as unknown as Mock).mockReturnValue(true);

    render(<RootErrorBoundary />);
    expect(screen.getByText("Looks like our API is down")).toBeInTheDocument();
  });

  it("shows generic message for non-route errors", () => {
    (useRouteError as Mock).mockReturnValue(new Error("Something broke"));
    (isRouteErrorResponse as unknown as Mock).mockReturnValue(false);

    render(<RootErrorBoundary />);
    expect(screen.getByText("Something went wrong")).toBeInTheDocument();
  });
});
