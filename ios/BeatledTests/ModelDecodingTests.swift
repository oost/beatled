import XCTest
@testable import Beatled

/// Decodes sample payloads matching `docs/api.markdown` and asserts the
/// Swift models pick up every consumer-facing field. The fixtures include
/// fields the models intentionally ignore (e.g. `qos`) so a server-side
/// addition can't break decoding.
final class ModelDecodingTests: XCTestCase {
    private func decode<T: Decodable>(_ type: T.Type, _ json: String) throws -> T {
        try JSONDecoder().decode(type, from: Data(json.utf8))
    }

    // MARK: /api/status

    func testStatusResponseDecoding() throws {
        let json = """
        {
          "message": "It's all good!",
          "status": {
            "beat-detector": true,
            "manual-bpm": false,
            "udp-server": false,
            "tempo-broadcaster": true
          },
          "tempo": 120.5,
          "manualBpm": 120.0,
          "deviceCount": 3
        }
        """
        let status = try decode(StatusResponse.self, json)
        XCTAssertEqual(status.message, "It's all good!")
        XCTAssertEqual(status.tempo, 120.5)
        XCTAssertEqual(status.deviceCount, 3)
        XCTAssertEqual(status.status["beat-detector"], true)
        XCTAssertEqual(status.status["manual-bpm"], false)
        XCTAssertEqual(status.status["tempo-broadcaster"], true)
        XCTAssertEqual(status.status.count, 4)
        XCTAssertEqual(status.manualBpm, 120.0)
    }

    func testStatusResponseDecodesWithoutManualBpm() throws {
        // Older servers omit manualBpm; the field is optional so decoding
        // must still succeed.
        let json = """
        {
          "message": "ok",
          "status": { "beat-detector": true },
          "tempo": 120.5,
          "deviceCount": 0
        }
        """
        let status = try decode(StatusResponse.self, json)
        XCTAssertNil(status.manualBpm)
    }

    // MARK: /api/devices

    func testDevicesResponseDecoding() throws {
        // The wire format is snake_case (`last_status_time` is the canonical
        // name — `last_seen` was the historical mistake renamed away).
        let json = """
        {
          "devices": [
            {
              "client_id": 1,
              "board_id": "E6614103E72B6A2F",
              "ip_address": "192.168.1.42",
              "last_status_time": 1707900120000000,
              "port_name": "pico-freertos",
              "git_sha": "1a2b3c4-dirty",
              "qos": { "current_offset_us": -42, "last_rtt_us": 555 }
            },
            {
              "client_id": 2,
              "board_id": "E6614103E72B6A30",
              "ip_address": "192.168.1.43",
              "last_status_time": 0
            }
          ],
          "count": 2
        }
        """
        let response = try decode(DevicesResponse.self, json)
        XCTAssertEqual(response.count, 2)
        XCTAssertEqual(response.devices.count, 2)

        let device = try XCTUnwrap(response.devices.first)
        XCTAssertEqual(device.clientId, 1)
        XCTAssertEqual(device.boardId, "E6614103E72B6A2F")
        XCTAssertEqual(device.ipAddress, "192.168.1.42")
        XCTAssertEqual(device.lastStatusTime, 1_707_900_120_000_000)
        XCTAssertEqual(device.id, 1, "Identifiable id should mirror client_id")
        XCTAssertEqual(device.portName, "pico-freertos")
        XCTAssertEqual(device.gitSha, "1a2b3c4-dirty")
        XCTAssertEqual(device.qos?.currentOffsetUs, -42)
        XCTAssertEqual(device.qos?.lastRttUs, 555)
        XCTAssertNil(device.qos?.medianRttUs, "absent QoS fields decode as nil")

        let bare = try XCTUnwrap(response.devices.last)
        XCTAssertNil(bare.portName)
        XCTAssertNil(bare.qos, "qos is nil until the first v4 snapshot")
    }

    // MARK: /api/qos

    func testFleetQosDecoding() throws {
        // Fixture mirrors the GET /api/qos example in docs/api.markdown.
        let json = """
        {
          "device_count": 2,
          "reporting_count": 2,
          "min_offset_us": -42,
          "max_offset_us": 17,
          "fleet_skew_us": 59,
          "mean_rtt_us": 1234,
          "min_rtt_us": 1100,
          "max_rtt_us": 1400,
          "slowest_device_board_id": "E6614103E72B6A2F",
          "total_next_beat_gap": 3,
          "total_intercore_drops": 0,
          "total_time_sync_outliers": 5,
          "thresholds": { "skew_warn_us": 5000, "skew_fail_us": 20000 },
          "health": "ok"
        }
        """
        let qos = try decode(FleetQos.self, json)
        XCTAssertEqual(qos.deviceCount, 2)
        XCTAssertEqual(qos.reportingCount, 2)
        XCTAssertEqual(qos.fleetSkewUs, 59)
        XCTAssertEqual(qos.meanRttUs, 1234)
        XCTAssertEqual(qos.maxRttUs, 1400)
        XCTAssertEqual(qos.slowestDeviceBoardId, "E6614103E72B6A2F")
        XCTAssertEqual(qos.totalNextBeatGap, 3)
        XCTAssertEqual(qos.totalIntercoreDrops, 0)
        XCTAssertEqual(qos.totalTimeSyncOutliers, 5)
        XCTAssertEqual(qos.health, "ok")
        XCTAssertEqual(qos.thresholds.skewWarnUs, 5000)
        XCTAssertEqual(qos.thresholds.skewFailUs, 20000)
    }

    func testFleetQosDecodesNullAggregates() throws {
        // Aggregates are null until any device reports a snapshot.
        let json = """
        {
          "device_count": 1,
          "reporting_count": 0,
          "min_offset_us": null,
          "max_offset_us": null,
          "fleet_skew_us": null,
          "mean_rtt_us": null,
          "min_rtt_us": null,
          "max_rtt_us": null,
          "slowest_device_board_id": "",
          "total_next_beat_gap": 0,
          "total_intercore_drops": 0,
          "total_time_sync_outliers": 0,
          "thresholds": { "skew_warn_us": 5000, "skew_fail_us": 20000 },
          "health": "unknown"
        }
        """
        let qos = try decode(FleetQos.self, json)
        XCTAssertNil(qos.fleetSkewUs)
        XCTAssertNil(qos.meanRttUs)
        XCTAssertEqual(qos.health, "unknown")
    }

    func testDeviceRejectsLegacyLastSeenKey() {
        // A payload still using the pre-rename `last_seen` key must fail to
        // decode — silently accepting it would hide a server regression.
        let json = """
        {
          "client_id": 1,
          "board_id": "E6614103E72B6A2F",
          "ip_address": "192.168.1.42",
          "last_seen": 1707900120000000
        }
        """
        XCTAssertThrowsError(try decode(Device.self, json))
    }

    // MARK: Device.lastSeenText

    private func device(lastStatusTime: UInt64) -> Device {
        Device(clientId: 1,
               boardId: "B",
               ipAddress: "192.168.1.42",
               lastStatusTime: lastStatusTime)
    }

    private func microsecondsAgo(_ seconds: TimeInterval) -> UInt64 {
        UInt64((Date().timeIntervalSince1970 - seconds) * 1_000_000)
    }

    func testLastSeenTextIsUnknownForZeroAndFutureTimestamps() {
        XCTAssertEqual(device(lastStatusTime: 0).lastSeenText, "unknown")
        let future = microsecondsAgo(-3600)
        XCTAssertEqual(device(lastStatusTime: future).lastSeenText, "unknown")
    }

    func testLastSeenTextBuckets() {
        XCTAssertEqual(device(lastStatusTime: microsecondsAgo(30)).lastSeenText, "30s ago")
        XCTAssertEqual(device(lastStatusTime: microsecondsAgo(5 * 60)).lastSeenText, "5m ago")
        XCTAssertEqual(device(lastStatusTime: microsecondsAgo(2 * 3600)).lastSeenText, "2h ago")
    }

    // MARK: /api/program

    func testProgramResponseDecoding() throws {
        let json = """
        {
          "message": "Current program is 2",
          "programId": 2,
          "programs": [
            { "name": "Snakes!", "id": 0 },
            { "name": "Random data", "id": 1 },
            { "name": "Sparkles", "id": 2 }
          ]
        }
        """
        let response = try decode(ProgramResponse.self, json)
        XCTAssertEqual(response.message, "Current program is 2")
        XCTAssertEqual(response.programId, 2)
        XCTAssertEqual(response.programs.count, 3)
        XCTAssertEqual(response.programs[2].name, "Sparkles")
        XCTAssertEqual(response.programs[2].id, 2)
    }

    // MARK: /api/service/control

    func testServiceControlRoundTrip() throws {
        let request = ServiceControlRequest(id: "udp-server", status: true)
        let encoded = try JSONEncoder().encode(request)
        let object = try XCTUnwrap(
            JSONSerialization.jsonObject(with: encoded) as? [String: Any])
        XCTAssertEqual(object["id"] as? String, "udp-server")
        XCTAssertEqual(object["status"] as? Bool, true)

        let response = try decode(ServiceControlResponse.self, #"{ "status": true }"#)
        XCTAssertTrue(response.status)
    }

    // MARK: /api/ap

    func testAPControlRequestEncoding() throws {
        let request = APControlRequest(mode: "on", revertMinutes: 10)
        let encoded = try JSONEncoder().encode(request)
        let object = try XCTUnwrap(
            JSONSerialization.jsonObject(with: encoded) as? [String: Any])
        XCTAssertEqual(object["mode"] as? String, "on")
        XCTAssertEqual(object["revertMinutes"] as? Int, 10)
    }

    func testAPControlRequestOmitsRevertWhenNil() throws {
        let request = APControlRequest(mode: "status", revertMinutes: nil)
        let encoded = try JSONEncoder().encode(request)
        let object = try XCTUnwrap(
            JSONSerialization.jsonObject(with: encoded) as? [String: Any])
        XCTAssertEqual(object["mode"] as? String, "status")
        XCTAssertNil(object["revertMinutes"])
    }

    func testAPStatusResponseDecoding() throws {
        let response = try decode(APStatusResponse.self, #"{ "ap": "on" }"#)
        XCTAssertEqual(response.ap, "on")
    }

    func testAPActionResponseDecoding() throws {
        let response = try decode(
            APActionResponse.self,
            #"{ "result": "ok", "mode": "off", "output": "Reconnecting to WiFi 'Livebox-98D4'..." }"#)
        XCTAssertEqual(response.result, "ok")
        XCTAssertEqual(response.mode, "off")
        XCTAssertTrue(response.output.contains("Reconnecting"))
    }
}
