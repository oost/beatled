import Foundation

@Observable
class ProgramViewModel {
    var programs: [Program] = []
    var selectedProgramId: Int = 0
    var error: String?
    var isLoading = false

    private let api: APIClient

    init(api: APIClient) {
        self.api = api
    }

    func load() {
        Task { @MainActor in
            isLoading = true
            defer { isLoading = false }
            do {
                let response: ProgramResponse = try await api.get("/api/program")
                self.programs = response.programs
                self.selectedProgramId = response.programId
                self.error = nil
            } catch {
                self.error = error.localizedDescription
            }
        }
    }

    func selectProgram(_ id: Int) {
        Task { @MainActor in
            do {
                struct Body: Encodable { let programId: Int }
                struct Response: Decodable { let message: String }
                let _: Response = try await api.post("/api/program", body: Body(programId: id))
                self.selectedProgramId = id
                self.error = nil
            } catch {
                self.error = error.localizedDescription
            }
        }
    }
}
