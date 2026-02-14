---
title: Beatled Controller
layout: default
nav_order: 6
---

# Beatled Web Client

A React single-page application for monitoring and controlling the Beatled system from your phone or browser.

![Image](/beatled/assets/images/client.png){: width="350" }

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Framework | React 18 + TypeScript |
| Routing | React Router v6 (data loaders) |
| Styling | Tailwind CSS 4 + shadcn/ui |
| Charts | Chart.js + react-chartjs-2 |
| Build | Vite |
| PWA | vite-plugin-pwa (offline support) |

## Views

| View | Path | Description |
|------|------|-------------|
| Status | `/status` | Real-time tempo display, beat history chart, service controls, connected devices |
| Program | `/program` | Select the active LED pattern across all devices |
| Log | `/log` | Server log viewer and client console log |
| Config | `/config` | API host selection and authentication token |

## Local Development

```bash
# Start the Vite dev server with hot module replacement
utils/beatled.sh client

# Production build
utils/beatled.sh client-build

# Run tests
cd client && npm test

# Lint and format check
cd client && npm run lint && npm run format:check
```

The dev server proxies `/api` requests to `https://127.0.0.1:8443` so you can run the server and client in separate terminals.

## Deployment

The client is built as static assets and served by the beat server over HTTPS. The Docker build handles this automatically. In production, access the app at:

```
https://raspberrypi.local:8443/
```

Replace the hostname with your Raspberry Pi's local domain name.

## PWA Support

The app is a Progressive Web App and can be installed on mobile devices. It is served over HTTPS to enable this. Self-signed certificates will cause browser security warnings -- Safari on macOS may refuse to load API endpoints with self-signed certs.
