import { describe, it, expect } from "vitest";
import { render, screen } from "@testing-library/react";
import PageHeader from "../page-header";

describe("PageHeader", () => {
  it("renders the title", () => {
    render(<PageHeader title="Test Title" />);
    expect(screen.getByText("Test Title")).toBeInTheDocument();
  });

  it("renders as an h1 element", () => {
    render(<PageHeader title="Status" />);
    const heading = screen.getByRole("heading", { level: 1 });
    expect(heading).toHaveTextContent("Status");
  });
});
