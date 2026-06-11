import { describe, it, expect, vi, beforeEach, type Mock } from "vitest";
import { getProgram, postProgram } from "../program";

// Mock only the fetch helpers; the error-classification helpers
// (ApiError, toApiFailure, ...) stay real.
vi.mock("../api", async (importOriginal) => ({
  ...(await importOriginal<typeof import("../api")>()),
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
}));

import { getEndpoint, postEndpoint, ApiError } from "../api";

describe("getProgram", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("returns program data on success", async () => {
    const programData = {
      programId: 2,
      programs: [
        { name: "Snakes!", id: 0 },
        { name: "Sparkles", id: 2 },
      ],
    };
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve(programData),
    });

    const result = await getProgram();

    expect(getEndpoint).toHaveBeenCalledWith("/api/program");
    expect(result).toEqual(programData);
  });

  it("returns error object on failure", async () => {
    (getEndpoint as Mock).mockRejectedValue(new Error("Timeout"));

    const result = await getProgram();

    expect(result).toEqual({ error: true, kind: "network", status: "Network error" });
  });

  it("carries the HTTP status code through on http failure", async () => {
    (getEndpoint as Mock).mockRejectedValue(new ApiError("http", 503));

    const result = await getProgram();

    expect(result).toEqual({ error: true, kind: "http", httpStatus: 503, status: "HTTP 503" });
  });

  it("treats a malformed programs list as an error", async () => {
    (getEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ programs: [{ id: "zero", name: 0 }] }),
    });

    const result = await getProgram();

    expect(result).toEqual({ error: true, kind: "invalid", status: "Invalid server response" });
  });
});

describe("postProgram", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("posts program update", async () => {
    (postEndpoint as Mock).mockResolvedValue({
      json: () => Promise.resolve({ message: "Updated program to 5" }),
    });

    const result = await postProgram(5);

    expect(postEndpoint).toHaveBeenCalledWith("/api/program", { programId: 5 });
    expect((result as { message: string }).message).toBe("Updated program to 5");
  });

  it("returns error object on failure", async () => {
    (postEndpoint as Mock).mockRejectedValue(new Error("Error"));

    const result = await postProgram(5);

    expect(result).toEqual({ error: true, kind: "network", status: "Network error" });
  });
});
