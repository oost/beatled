---
title: iOS Controller
layout: default
parent: Components
nav_order: 4
---

# iOS Controller

Source code: [github.com/oost/beatled](https://github.com/oost/beatled) (`ios/`)

A native iOS app for monitoring and controlling the Beatled system. Built with SwiftUI, it provides the same functionality as the [Web Controller](client.html) in a native experience.

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Framework | SwiftUI |
| Minimum iOS | 17.0 |
| Charts | Swift Charts |
| Networking | URLSession (async/await) |
| Settings | UserDefaults |

## Views

The app uses a tab-based layout with four screens:

| Tab | Description |
|-----|-------------|
| Status | Real-time BPM display, tempo history chart, service toggles, connected devices table |
| Program | Browse and select the active LED pattern |
| Log | Server log viewer with auto-scroll and 10-second polling |
| Config | Server host selection with health checks, API token, TLS settings |

## Architecture

The app follows an MVVM pattern:

```
Views/           → SwiftUI views
ViewModels/      → @Observable view models with async polling
Services/        → APIClient (networking), AppSettings (persistence)
Models/          → Codable response types
```

Each view model polls its endpoint on a timer (2s for status, 10s for logs) and exposes loading/error state to the view.

## Configuration

The Config view provides preset server hosts:

| Preset | Address |
|--------|---------|
| beatled.local | `https://beatled.local:8443` |
| beatled.test | `https://beatled.test:8443` |
| Raspberry Pi | `https://raspberrypi.local:8443` |
| Localhost | `https://localhost:8443` |
| Vite Proxy | `https://localhost:5173` |
| Custom | User-defined URL |

Each preset shows a live health indicator. The app also supports:

- **API token** -- Bearer token authentication (stored in UserDefaults)
- **Insecure connections** -- toggle to accept self-signed certificates for local development

## Local Development

### Generate the Xcode project

The project file is generated from `project.yml` using [XcodeGen](https://github.com/yonaskolb/XcodeGen):

```bash
scripts/beatled.sh ios generate
```

### Build and run

```bash
# Open in Xcode
scripts/beatled.sh ios

# Or build from command line
scripts/beatled.sh ios build
```

### Self-Signed Certificates

For the iOS Simulator to trust self-signed mkcert certificates:

```bash
xcrun simctl keychain booted add-root-cert "$(mkcert -CAROOT)/rootCA.pem"
```

Alternatively, enable "Allow insecure connections" in the app's Config tab.
