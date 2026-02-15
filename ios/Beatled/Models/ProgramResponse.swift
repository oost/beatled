import Foundation

struct ProgramResponse: Codable {
    let message: String
    let programs: [Program]
    let programId: Int
}

struct Program: Codable, Identifiable {
    let id: Int
    let name: String
}
