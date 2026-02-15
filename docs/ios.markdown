---
title: iOS & macOS Controller
layout: default
parent: Components
nav_order: 4
---

# iOS & macOS Controller

Source code: [github.com/oost/beatled](https://github.com/oost/beatled) (`ios/`)

A native universal app for monitoring and controlling the Beatled system. Built with SwiftUI, it runs on both iOS and macOS and provides the same functionality as the [Web Controller](client.html) in a native experience.

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Framework | SwiftUI |
| Platforms | iOS 26+, macOS 26+ |
| Charts | Swift Charts |
| Networking | URLSession (async/await) |
| Settings | UserDefaults |

## Views

On iOS the app uses a tab bar; on macOS it uses a sidebar with `NavigationSplitView`. Both layouts expose the same four screens:

| Screen | Description |
|--------|-------------|
| Status | Real-time BPM display, tempo history chart, service toggles, connected devices table |
| Program | Browse and select the active LED pattern |
| Log | Server log viewer with auto-scroll and 10-second polling |
| Config | Server host selection with health checks, API token, TLS settings |

## Architecture

The app follows an MVVM pattern:

```
Views/           → SwiftUI views (+ MacContentView for sidebar)
ViewModels/      → @Observable view models with async polling
Services/        → APIClient (networking), AppSettings (persistence)
Models/          → Codable response types
```

Each view model polls its endpoint on a timer (2s for status, 10s for logs) and exposes loading/error state to the view. All views are shared between iOS and macOS — platform-specific code is limited to `ContentView` (tab bar vs sidebar) and a few `#if os()` guards for iOS-only modifiers.

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
xcodegen generate --spec ios/project.yml --project ios/
```

### Build and run

```bash
# Open in Xcode
scripts/beatled.sh ios

# Build iOS (simulator)
scripts/beatled.sh ios build

# Build and run in iOS Simulator
scripts/beatled.sh ios sim

# Build and run macOS app
scripts/beatled.sh ios mac
```

### Self-Signed Certificates

On the **iOS Simulator**, install the mkcert root CA so it trusts self-signed certificates:

```bash
xcrun simctl keychain booted add-root-cert "$(mkcert -CAROOT)/rootCA.pem"
```

On **macOS**, the mkcert root CA is installed in the system keychain automatically by `mkcert -install`.

Alternatively, enable "Allow insecure connections" in the app's Config screen on either platform.
