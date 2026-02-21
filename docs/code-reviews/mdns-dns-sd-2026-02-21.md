# mDNS / DNS-SD Implementation Review
*2026-02-21*

Supersedes: `mdns-service-discovery-2026-02-20.md` (initial notes)

---

## Problem

Controllers (ESP32 / Pico W) need to discover the server's IP address on the local WiFi network without manual configuration. The server runs as a C++ / RESTinio binary on macOS (dev) and Raspberry Pi / Linux (production).

---

## Architecture Overview

```
[Server: Mac/Pi]                [Controllers: ESP32 / Pico W]
  RESTinio (HTTP/WS)  <──WS──>  WebSocket client (commands/config)
  mDNS advertiser     <──────>  mDNS query on boot → resolve IP
  UDP multicast       ──────>   Beat sync (OSC or raw binary)
```

**Two protocols in play:**
| Path | Protocol | Why |
|---|---|---|
| Controller ↔ Server (control) | WebSocket `ws://` | Reliable, persistent, easy to implement |
| Controller ↔ Server (beat sync) | UDP multicast | Low latency, loss-tolerant, one packet to all controllers |
| iOS/Web client ↔ Server | WebSocket `wss://` | TLS via mkcert for `beatled.local` |

---

## Service Discovery: mDNS + DNS-SD

### Service type

Advertise as `_beatled._tcp.local.` on the WebSocket port. Controllers query by service type — they never need a hardcoded IP or hostname.

### Boot sequence

```
Server starts
  → advertise _beatled._tcp on port 8080

Controller boots
  → query _beatled._tcp.local via mDNS
  → receive: IP + port from PTR/SRV records
  → connect WebSocket to discovered address
  → join UDP multicast group for beat data
```

---

## Server Implementation (C++ / RESTinio, macOS + Linux)

### Library: `mdns.h` (single header)

**Why:** Cross-platform (macOS + Linux), no daemon dependency, no Avahi required on Pi. Uses raw POSIX multicast sockets directly.

- Repo: https://github.com/mjansson/mdns
- Integration: copy `mdns.h` into `third_party/mdns/`

### CMake

```cmake
target_include_directories(beatled-server PRIVATE third_party/mdns)

# No extra link flags needed on Linux (POSIX sockets)
# CoreServices not needed either — mdns.h doesn't use dns_sd.h
```

### Code structure

Run the mDNS responder in a dedicated thread alongside RESTinio's ASIO thread pool:

```cpp
#define MDNS_IMPLEMENTATION
#include "mdns.h"
#include <thread>
#include <atomic>
#include <cstring>

class MdnsAdvertiser {
public:
    MdnsAdvertiser(std::string service_name, uint16_t port)
        : service_name_(std::move(service_name)), port_(port) {}

    void start() {
        running_ = true;
        thread_ = std::thread(&MdnsAdvertiser::run, this);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

private:
    void run() {
        int sockets[8];
        int num = mdns_socket_open_ipv4(sockets, 8);
        if (num <= 0) return;

        // Initial announcement
        announce(sockets, num);

        uint8_t buf[2048];
        while (running_) {
            // Poll for incoming mDNS queries and respond
            for (int i = 0; i < num; i++) {
                mdns_socket_recv(sockets[i], buf, sizeof(buf),
                                 query_callback, this);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        for (int i = 0; i < num; i++)
            mdns_socket_close(sockets[i]);
    }

    void announce(int* sockets, int num) {
        // Send PTR + SRV + TXT records
        // See mdns.h mdns_service_announce example for full record setup
        // Records needed:
        //   PTR  _beatled._tcp.local.  -> <name>._beatled._tcp.local.
        //   SRV  <name>._beatled._tcp.local. -> <hostname>.local. port
        //   TXT  (optional: version, etc.)
        //   A    <hostname>.local. -> <IP>
    }

    static int query_callback(int sock, const struct sockaddr* from,
                               size_t addrlen, mdns_entry_type_t entry,
                               uint16_t query_id, uint16_t rtype,
                               uint16_t rclass, uint32_t ttl,
                               const void* data, size_t size,
                               size_t name_offset, size_t name_length,
                               size_t record_offset, size_t record_length,
                               void* user_data) {
        // Respond to PTR queries for _beatled._tcp.local.
        // See mdns_service_announce.c example
        return 0;
    }

    std::string service_name_;
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};
```

```cpp
int main() {
    const uint16_t port = 8080;

    MdnsAdvertiser mdns("Beatled Server", port);
    mdns.start();

    restinio::run(
        restinio::on_thread_pool<>(4)
            .port(port)
            .request_handler(router)
    );

    mdns.stop();
}
```

> **Note:** The `mdns.h` advertisement API requires sending responses to PTR queries. The library's `mdns_service_announce.c` example is the authoritative reference for correct PTR/SRV/TXT/A record construction. Use it directly — the callback plumbing is non-trivial to get right from scratch.

### Linux caveat: systemd-resolved conflict

On Raspberry Pi OS, `systemd-resolved` may hold port 5353. Check at startup:

```cpp
// If mdns_socket_open_ipv4() returns 0, port 5353 is likely taken.
// Workaround options:
//   1. sudo systemctl disable systemd-resolved (nuclear)
//   2. Edit /etc/systemd/resolved.conf: MulticastDNS=no, then restart
//   3. Run with CAP_NET_BIND_SERVICE capability
```

Document this in the dev setup guide.

---

## Controller Implementation

### ESP32

**Library:** `ESPmDNS` (Arduino framework) or `mdns` component (ESP-IDF)

```cpp
// Arduino / ESP-IDF Arduino
#include <ESPmDNS.h>

void discover_server(char* out_ip, uint16_t* out_port) {
    if (!MDNS.begin("controller")) return;  // own hostname (optional)

    // Query for beatled service
    int n = MDNS.queryService("beatled", "tcp");
    if (n > 0) {
        strncpy(out_ip, MDNS.IP(0).toString().c_str(), 64);
        *out_port = MDNS.port(0);
    }
}
```

With ESP-IDF directly:
```c
#include "mdns.h"  // ESP-IDF component, different from mjansson's

mdns_result_t* results = NULL;
esp_err_t err = mdns_query_ptr("_beatled._tcp", "_tcp", 3000, 10, &results);
if (err == ESP_OK && results) {
    // results->addr->addr.u_addr.ip4 → server IP
    // results->port → server port
    mdns_query_results_free(results);
}
```

### Pico W

**lwIP mDNS** — enable in `lwipopts.h`:

```c
#define LWIP_MDNS_RESPONDER  1
#define MDNS_MAX_SERVICES    4
```

Query side (mDNS browse):
```c
#include "lwip/apps/mdns.h"

// lwIP's mDNS browse API is limited — querying by service type
// requires sending a PTR query manually or using a thin wrapper.
// Alternative: hardcode resolution to "beatled.local" via mdns_gethostbyname()
// and use a fixed service name as a fallback.
mdns_gethostbyname("beatled.local", &server_ip, callback, arg);
```

> **Pico W note:** lwIP mDNS browse (querying by service type) is less mature than ESP32's. The fallback of resolving `beatled.local` directly is simpler and acceptable for a fixed single-server deployment.

---

## Beat Sync: UDP Multicast

The server sends beat events to a multicast group. All controllers receive simultaneously with no per-device addressing.

**Multicast group:** `239.0.0.1:9001` (site-local, administratively scoped)

### Packet format (raw binary, 10 bytes)

```c
struct __attribute__((packed)) BeatPacket {
    uint8_t  type;          // 0x01 = beat onset
    float    bpm;           // current BPM
    float    confidence;    // beat detector confidence [0.0, 1.0]
    // timestamp omitted — controllers fire immediately on receipt
};
```

**Alternative:** OSC over UDP — adds ecosystem interop (TouchOSC, DAWs) at the cost of a small library dependency. Use if external tool integration is wanted.

### Server send (C++)

```cpp
int sock = socket(AF_INET, SOCK_DGRAM, 0);
struct sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(9001);
inet_pton(AF_INET, "239.0.0.1", &addr.sin_addr);

BeatPacket pkt{ 0x01, bpm, confidence };
sendto(sock, &pkt, sizeof(pkt), 0,
       (struct sockaddr*)&addr, sizeof(addr));
```

### Controller receive (ESP32)

```cpp
WiFiUDP udp;
udp.beginMulticast(IPAddress(239,0,0,1), 9001);

void loop() {
    int len = udp.parsePacket();
    if (len == sizeof(BeatPacket)) {
        BeatPacket pkt;
        udp.read((uint8_t*)&pkt, sizeof(pkt));
        if (pkt.type == 0x01) fire_leds(pkt.bpm, pkt.confidence);
    }
}
```

---

## TLS (optional, future)

TLS between controllers and server is not recommended for initial implementation (cert embedding adds build complexity, cert rotation requires reflash).

For the iOS/web client path, use mkcert:

```bash
mkcert beatled.local
# Outputs: beatled.local.pem + beatled.local-key.pem
```

- macOS/iOS clients: `mkcert -install` + install CA profile in iOS Settings
- RESTinio TLS: configure with the generated cert/key pair

---

## Implementation Checklist

### Server
- [ ] Add `mdns.h` to `third_party/`
- [ ] Implement `MdnsAdvertiser` class (PTR/SRV/TXT response)
- [ ] Advertise `_beatled._tcp` on server startup
- [ ] Add UDP multicast sender for beat events
- [ ] Document `systemd-resolved` workaround in Pi setup guide

### ESP32
- [ ] Add mDNS query on WiFi connect
- [ ] Fall back to `beatled.local` if service query times out
- [ ] Join UDP multicast group `239.0.0.1:9001` after discovery

### Pico W
- [ ] Enable `LWIP_MDNS_RESPONDER` in `lwipopts.h`
- [ ] Resolve `beatled.local` via `mdns_gethostbyname()`
- [ ] Join UDP multicast group

### Testing
- [ ] Verify advertisement with `dns-sd -B _beatled._tcp local` from Mac
- [ ] Verify resolution with `dns-sd -G v4 beatled.local`
- [ ] Test discovery from ESP32 dev board before integrating into main firmware
- [ ] Test UDP multicast delivery to multiple simultaneous controllers

---

## References
- [mdns.h (mjansson)](https://github.com/mjansson/mdns) — single-header C mDNS library
- [mdns_service_announce.c](https://github.com/mjansson/mdns/blob/master/mdns_service_announce.c) — authoritative example for PTR/SRV/TXT record construction
- [ESP-IDF mDNS component](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html)
- [lwIP mDNS](https://www.nongnu.org/lwip/2_1_x/group__mdns.html)
- RFC 6762 (mDNS), RFC 6763 (DNS-SD)
