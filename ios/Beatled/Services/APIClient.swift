import Foundation

enum APIError: LocalizedError {
    case unauthorized
    case networkError(Error)
    case decodingError(Error)
    case serverError(Int, String)
    case invalidURL

    var errorDescription: String? {
        switch self {
        case .unauthorized:
            return "Unauthorized — check your API token"
        case .networkError(let error):
            return "Network error: \(error.localizedDescription)"
        case .decodingError(let error):
            return "Decoding error: \(error.localizedDescription)"
        case .serverError(let code, let message):
            return "Server error \(code): \(message)"
        case .invalidURL:
            return "Invalid URL"
        }
    }
}

@Observable
class APIClient: NSObject {
    private let settings: AppSettings
    private var _session: URLSession?
    
    private var session: URLSession {
        if let _session {
            return _session
        }
        let newSession = URLSession(configuration: .default, delegate: self, delegateQueue: nil)
        _session = newSession
        return newSession
    }

    init(settings: AppSettings) {
        self.settings = settings
    }

    func get<T: Decodable>(_ endpoint: String) async throws -> T {
        let url = try buildURL(endpoint)
        var request = URLRequest(url: url)
        request.httpMethod = "GET"
        applyAuth(&request)
        return try await perform(request)
    }

    func post<T: Decodable>(_ endpoint: String, body: some Encodable) async throws -> T {
        let url = try buildURL(endpoint)
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try JSONEncoder().encode(body)
        applyAuth(&request)
        return try await perform(request)
    }

    func getRaw(_ endpoint: String) async throws -> Data {
        let url = try buildURL(endpoint)
        var request = URLRequest(url: url)
        request.httpMethod = "GET"
        applyAuth(&request)

        let (data, response) = try await session.data(for: request)
        guard let http = response as? HTTPURLResponse else {
            throw APIError.networkError(URLError(.badServerResponse))
        }
        if http.statusCode == 401 { throw APIError.unauthorized }
        guard (200...299).contains(http.statusCode) else {
            let body = String(data: data, encoding: .utf8) ?? ""
            throw APIError.serverError(http.statusCode, body)
        }
        return data
    }

    private func buildURL(_ endpoint: String) throws -> URL {
        guard let url = URL(string: settings.apiHost + endpoint) else {
            throw APIError.invalidURL
        }
        return url
    }

    private func applyAuth(_ request: inout URLRequest) {
        let token = settings.apiToken
        if !token.isEmpty {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }
    }

    private func perform<T: Decodable>(_ request: URLRequest) async throws -> T {
        let (data, response): (Data, URLResponse)
        do {
            (data, response) = try await session.data(for: request)
        } catch {
            throw APIError.networkError(error)
        }

        guard let http = response as? HTTPURLResponse else {
            throw APIError.networkError(URLError(.badServerResponse))
        }

        if http.statusCode == 401 { throw APIError.unauthorized }

        guard (200...299).contains(http.statusCode) else {
            let body = String(data: data, encoding: .utf8) ?? ""
            throw APIError.serverError(http.statusCode, body)
        }

        do {
            return try JSONDecoder().decode(T.self, from: data)
        } catch {
            print("❌ Decoding error for \(T.self):")
            print("   Error: \(error)")
            if let decodingError = error as? DecodingError {
                switch decodingError {
                case .keyNotFound(let key, let context):
                    print("   Missing key '\(key.stringValue)' - \(context.debugDescription)")
                case .typeMismatch(let type, let context):
                    print("   Type mismatch for type '\(type)' - \(context.debugDescription)")
                case .valueNotFound(let type, let context):
                    print("   Value not found for type '\(type)' - \(context.debugDescription)")
                case .dataCorrupted(let context):
                    print("   Data corrupted - \(context.debugDescription)")
                @unknown default:
                    print("   Unknown decoding error")
                }
            }
            if let jsonString = String(data: data, encoding: .utf8) {
                print("   Raw JSON: \(jsonString)")
            }
            throw APIError.decodingError(error)
        }
    }
}

extension APIClient: URLSessionDelegate {
    func urlSession(
        _ session: URLSession,
        didReceive challenge: URLAuthenticationChallenge,
        completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void
    ) {
        // Only bypass certificate validation if user has explicitly enabled insecure connections
        if settings.allowInsecureConnections,
           challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
           let trust = challenge.protectionSpace.serverTrust {
            completionHandler(.useCredential, URLCredential(trust: trust))
        } else {
            completionHandler(.performDefaultHandling, nil)
        }
    }
}
