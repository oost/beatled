import { describe, it, expect, vi, beforeEach } from "vitest";
import { getProgram, postProgram } from "../program";

vi.mock("../api", () => ({
  getEndpoint: vi.fn(),
  postEndpoint: vi.fn(),
  getAPIHost: vi.fn(() => "https://localhost:8080"),
  setAPIHost: vi.fn(),
}));

import { getEndpoint, postEndpoint } from "../api";

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
    getEndpoint.mockResolvedValue({
      json: () => Promise.resolve(programData),
    });

    const result = await getProgram();

    expect(getEndpoint).toHaveBeenCalledWith("/api/program");
    expect(result).toEqual(programData);
  });

  it("returns error object on failure", async () => {
    getEndpoint.mockRejectedValue(new Error("Timeout"));

    const result = await getProgram();

    expect(result).toEqual({ error: true, status: "Network error" });
  });
});

describe("postProgram", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("posts program update", async () => {
    postEndpoint.mockResolvedValue({
      json: () => Promise.resolve({ message: "Updated program to 5" }),
    });

    const result = await postProgram(5);

    expect(postEndpoint).toHaveBeenCalledWith("/api/program", { programId: 5 });
    expect(result.message).toBe("Updated program to 5");
  });

  it("returns error object on failure", async () => {
    postEndpoint.mockRejectedValue(new Error("Error"));

    const result = await postProgram(5);

    expect(result).toEqual({ error: true, status: "Network error" });
  });
});
