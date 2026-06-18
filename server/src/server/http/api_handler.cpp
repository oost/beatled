#include <array>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <sys/wait.h>

#include "./api_handler.hpp"
#include "beatled/protocol.h"

using json = nlohmann::json;
using beatled::core::tempo_ref_t;

namespace beatled::server {

namespace {
struct CommandResult {
  int exit_code;
  std::string output;
};

// Run a shell command, capturing stdout+stderr and the exit code. Used only
// for the AP-mode toggle (ap-mode.sh). The single caller-controlled part of
// the command line is whitelisted (mode) or an integer (revert minutes), so
// there is no shell-injection surface.
CommandResult run_command(const std::string &cmd) {
  std::array<char, 256> buf{};
  std::string out;
  FILE *pipe = ::popen(cmd.c_str(), "r");
  if (pipe == nullptr) {
    return {-1, "failed to start command"};
  }
  while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
    out += buf.data();
  }
  int status = ::pclose(pipe);
  int code = (status == -1) ? -1 : (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
  return {code, out};
}

std::string rstrip(std::string s) {
  while (!s.empty() &&
         (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
    s.pop_back();
  }
  return s;
}
} // namespace

struct Program {
  std::string name;
  int id;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Program, name, id)

APIHandler::APIHandler(ServiceManagerInterface &service_manager, Logger &logger,
                       const std::string &cors_origin, const std::string &api_token,
                       QosThresholds qos_thresholds, const std::string &ap_script)
    : service_manager_{service_manager}, logger_{logger}, cors_origin_{cors_origin},
      api_token_{api_token}, qos_thresholds_{qos_thresholds}, ap_script_{ap_script} {}

bool APIHandler::authorize_request(const std::optional<std::string> &authorization_header,
                                   std::string_view api_token) {
  if (api_token.empty()) {
    return true;
  }
  if (!authorization_header.has_value()) {
    return false;
  }
  // Constant-string comparison; no time-safety needed here since the token
  // is sent in cleartext over the (assumed-TLS) header anyway.
  return *authorization_header == std::string{"Bearer "} + std::string{api_token};
}

bool APIHandler::check_auth(const req_handle_t &req) const {
  auto auth_opt = req->header().opt_value_of(restinio::http_field::authorization);
  std::optional<std::string> auth_header;
  if (auth_opt.has_value()) {
    auth_header = std::string{*auth_opt};
  }
  return authorize_request(auth_header, api_token_);
}

bool APIHandler::check_rate_limit() {
  std::lock_guard lk(rate_mtx_);
  auto now = std::chrono::steady_clock::now();
  while (!request_times_.empty() && (now - request_times_.front()) > kWindowDuration) {
    request_times_.pop_front();
  }
  if (request_times_.size() >= kMaxRequestsPerWindow) {
    return false;
  }
  request_times_.push_back(now);
  return true;
}

APIHandler::req_status_t APIHandler::on_get_status(const req_handle_t &req, route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  json response_body;
  response_body["message"] = "It's all good!";

  json service_status;
  for (auto &&[first, second] : service_manager_.services()) {
    service_status[second->id()] = second->is_running();
  }
  response_body["status"] = service_status;
  response_body["tempo"] = service_manager_.state_manager().get_tempo_ref().tempo;
  response_body["manualBpm"] = service_manager_.state_manager().get_manual_bpm();
  response_body["deviceCount"] = service_manager_.state_manager().get_clients().size();
  response_body["uptime_us"] = service_manager_.uptime_us();

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_post_service_control(const req_handle_t &req,
                                                             route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }
  if (!check_rate_limit()) {
    return init_resp(req->create_response(restinio::status_too_many_requests()))
        .set_body(R"({"error":"Too many requests"})")
        .done();
  }

  json response_body;

  try {
    if (req->body().size() > 4096) {
      response_body["error"] = "Request body too large";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    json request_body = json::parse(req->body());
    if (!request_body.contains("id") || !request_body.contains("status")) {
      response_body["error"] = "Missing required fields: id, status";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    std::string service_name = request_body["id"].get<std::string>();
    bool requested_status = request_body["status"].get<bool>();
    ServiceControllerInterface *service = service_manager_.service(service_name);

    if (service) {
      if (requested_status) {
        // The audio beat detector and the manual-BPM metronome are mutually
        // exclusive tempo sources — running both would interleave two beat
        // streams onto the fleet. Starting one stops the other.
        const std::string other_source = service_name == "beat-detector" ? "manual-bpm"
                                         : service_name == "manual-bpm"  ? "beat-detector"
                                                                         : "";
        if (!other_source.empty()) {
          if (ServiceControllerInterface *other = service_manager_.service(other_source)) {
            other->stop();
          }
        }
        service->start();
      } else {
        service->stop();
      }
      response_body["status"] = service->is_running();

      return init_resp(req->create_response(restinio::status_ok()))
          .set_body(response_body.dump())
          .done();

    } else {
      response_body["error"] = "Not found";

      return init_resp(req->create_response(restinio::status_not_found()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
  } catch (const std::exception &e) {
    response_body["error"] = e.what();

    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_post_ap(const req_handle_t &req, route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }
  if (!check_rate_limit()) {
    return init_resp(req->create_response(restinio::status_too_many_requests()))
        .set_body(R"({"error":"Too many requests"})")
        .done();
  }

  json response_body;
  try {
    if (req->body().size() > 4096) {
      response_body["error"] = "Request body too large";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    json request_body = json::parse(req->body());
    if (!request_body.contains("mode")) {
      response_body["error"] = "Missing required field: mode";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    std::string mode = request_body["mode"].get<std::string>();
    // Whitelist: mode is part of the command line, so restrict it to these
    // three tokens to keep the shell-out injection-free.
    if (mode != "on" && mode != "off" && mode != "status") {
      response_body["error"] = "Invalid mode (expected on, off, or status)";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }

    std::string cmd = ap_script_ + " " + mode;
    // Optional auto-revert (only meaningful for "on"): an integer number of
    // minutes after which the Pi switches back to WiFi. It arrives as a JSON
    // number and is re-serialized via std::to_string, so it can only be
    // digits — no injection.
    if (mode == "on" && request_body.contains("revertMinutes")) {
      int revert = request_body["revertMinutes"].get<int>();
      if (revert < 0 || revert > 1440) {
        response_body["error"] = "revertMinutes out of range (0-1440)";
        return init_resp(req->create_response(restinio::status_bad_request()))
            .set_body(response_body.dump())
            .connection_close()
            .done();
      }
      if (revert > 0) {
        cmd += " " + std::to_string(revert);
      }
    }

    SPDLOG_INFO("AP mode request: {}", cmd);
    // NOTE: "on" switches wlan0 to AP mode, tearing down the link this request
    // arrived over — the client must rejoin the hotspot to see any response.
    // The handler still completes server-side.
    CommandResult result = run_command(cmd + " 2>&1");
    std::string out = rstrip(result.output);

    if (result.exit_code != 0) {
      response_body["error"] = "ap-mode.sh failed";
      response_body["mode"] = mode;
      response_body["exitCode"] = result.exit_code;
      response_body["output"] = out;
      SPDLOG_ERROR("ap-mode.sh failed (exit {}): {}", result.exit_code, out);
      return init_resp(req->create_response(restinio::status_internal_server_error()))
          .set_body(response_body.dump())
          .done();
    }

    if (mode == "status") {
      response_body["ap"] = out; // "on" or "off"
    } else {
      response_body["result"] = "ok";
      response_body["mode"] = mode;
      response_body["output"] = out;
    }
    return init_resp(req->create_response(restinio::status_ok()))
        .set_body(response_body.dump())
        .done();
  } catch (const std::exception &e) {
    response_body["error"] = e.what();
    SPDLOG_ERROR("Error with AP request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_get_tempo(const req_handle_t &req, route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  tempo_ref_t tr = service_manager_.state_manager().get_tempo_ref();

  json response_body;

  response_body["tempo"] = tr.tempo;
  response_body["time_ref"] = tr.beat_time_ref;
  response_body["manualBpm"] = service_manager_.state_manager().get_manual_bpm();

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_post_manual_tempo(const req_handle_t &req,
                                                          route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }
  if (!check_rate_limit()) {
    return init_resp(req->create_response(restinio::status_too_many_requests()))
        .set_body(R"({"error":"Too many requests"})")
        .done();
  }

  json response_body;
  try {
    if (req->body().size() > 4096) {
      response_body["error"] = "Request body too large";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    json request_body = json::parse(req->body());
    if (!request_body.contains("bpm")) {
      response_body["error"] = "Missing required field: bpm";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    const auto &bpm_field = request_body["bpm"];
    if (!bpm_field.is_number()) {
      response_body["error"] = "Field 'bpm' must be a number";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    double bpm = bpm_field.get<double>();
    // Keep in lockstep with ManualTempo's clamp range so the value the UI
    // reads back matches what the metronome will actually use.
    if (bpm < 20.0 || bpm > 400.0) {
      response_body["error"] = "Field 'bpm' must be between 20 and 400";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    service_manager_.state_manager().set_manual_bpm(static_cast<float>(bpm));

    response_body["manualBpm"] = service_manager_.state_manager().get_manual_bpm();
    return init_resp(req->create_response(restinio::status_ok()))
        .set_body(response_body.dump())
        .done();
  } catch (const std::exception &e) {
    response_body["error"] = e.what();
    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_post_program(const req_handle_t &req,
                                                     route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }
  if (!check_rate_limit()) {
    return init_resp(req->create_response(restinio::status_too_many_requests()))
        .set_body(R"({"error":"Too many requests"})")
        .done();
  }

  json response_body;
  try {
    if (req->body().size() > 4096) {
      response_body["error"] = "Request body too large";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    json request_body = json::parse(req->body());
    if (!request_body.contains("programId")) {
      response_body["error"] = "Missing required field: programId";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    // Per-field type validation. nlohmann::json::get<uint16_t> throws a
    // generic type_error if the field is the wrong type or out of range,
    // which previously surfaced to clients as a generic 400 with the
    // library's own message ("[json.exception.type_error.302] type must
    // be number, but is …"). Validate explicitly so we can return a
    // specific, debuggable error.
    const auto &program_id_field = request_body["programId"];
    if (!program_id_field.is_number_unsigned()) {
      response_body["error"] = "Field 'programId' must be a non-negative integer";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    uint64_t raw = program_id_field.get<std::uint64_t>();
    if (raw > std::numeric_limits<std::uint16_t>::max()) {
      response_body["error"] = "Field 'programId' must fit in a uint16 (0..65535)";
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
    uint16_t program_id = static_cast<std::uint16_t>(raw);
    service_manager_.state_manager().update_program_id(program_id);

    response_body["message"] = fmt::format("Updated program to {}", program_id);

    return init_resp(req->create_response(restinio::status_ok()))
        .set_body(response_body.dump())
        .done();
  } catch (const std::exception &e) {
    response_body["error"] = e.what();

    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_get_program(const req_handle_t &req,
                                                    route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  uint16_t program_id = service_manager_.state_manager().get_program_id();

  json response_body;
  response_body["message"] = fmt::format("Current program is {}", program_id);

  // Expanded from the shared protocol header — the same table the firmware
  // builds its pattern functions from, so the two lists can't drift apart.
  json programs = json::array();
#define X(id, fn_suffix, display_name) programs.push_back(Program{display_name, id});
  BEATLED_PROGRAM_TABLE(X)
#undef X
  response_body["programs"] = std::move(programs);

  response_body["programId"] = program_id;

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_get_log(const req_handle_t &req, route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(logger_.log_tail().dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_get_devices(const req_handle_t &req,
                                                    route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  auto clients = service_manager_.state_manager().get_clients();

  json devices = json::array();
  for (const auto &cs : clients) {
    json device = *cs;
    // Add ip_address (not included in NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE)
    device["ip_address"] = cs->ip_address.to_string();
    devices.push_back(device);
  }

  json response_body;
  response_body["devices"] = devices;
  response_body["count"] = clients.size();

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_get_qos(const req_handle_t &req, route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  // Snapshot the client list once and compute fleet-wide aggregates over
  // the current ClientStatus::latest_qos values. Devices that haven't
  // reported a QoS snapshot yet are skipped from the sync/skew math but
  // still counted toward `device_count` / `reporting_count`.
  auto clients = service_manager_.state_manager().get_clients();
  size_t reporting = 0;
  int64_t min_offset_us = std::numeric_limits<int64_t>::max();
  int64_t max_offset_us = std::numeric_limits<int64_t>::min();
  // Fleet skew is the spread of per-device *sync errors* (see
  // qos_sync_error_us): raw offsets are dominated by each device's boot
  // epoch, so their spread measures who booted when, not sync quality.
  size_t skew_reporting = 0;
  int64_t min_sync_error_us = std::numeric_limits<int64_t>::max();
  int64_t max_sync_error_us = std::numeric_limits<int64_t>::min();
  uint64_t min_rtt_us = std::numeric_limits<uint64_t>::max();
  uint64_t max_rtt_us = 0;
  uint64_t sum_rtt_us = 0;
  uint64_t total_next_beat_gap = 0;
  uint64_t total_intercore_drops = 0;
  uint64_t total_time_sync_outliers = 0;
  std::string slowest_id;
  for (const auto &cs : clients) {
    if (!cs->latest_qos.valid) {
      continue;
    }
    ++reporting;
    const auto &qos = cs->latest_qos;
    if (qos.current_offset_us < min_offset_us)
      min_offset_us = qos.current_offset_us;
    if (qos.current_offset_us > max_offset_us)
      max_offset_us = qos.current_offset_us;
    int64_t sync_error_us = 0;
    if (core::qos_sync_error_us(qos, sync_error_us)) {
      ++skew_reporting;
      if (sync_error_us < min_sync_error_us)
        min_sync_error_us = sync_error_us;
      if (sync_error_us > max_sync_error_us)
        max_sync_error_us = sync_error_us;
    }
    if (qos.median_rtt_us > 0) {
      if (qos.median_rtt_us < min_rtt_us)
        min_rtt_us = qos.median_rtt_us;
      if (qos.median_rtt_us > max_rtt_us) {
        max_rtt_us = qos.median_rtt_us;
        // Best-effort identifier for the operator: prefer board_id over
        // raw client_id since the React Devices table renders board_id.
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; ++i) {
          oss << std::setw(2)
              << static_cast<unsigned int>(static_cast<unsigned char>(cs->board_id[i]));
        }
        slowest_id = oss.str();
      }
      sum_rtt_us += qos.median_rtt_us;
    }
    total_next_beat_gap += qos.next_beat_gap_total;
    total_intercore_drops += qos.intercore_drop_total;
    total_time_sync_outliers += qos.time_sync_outlier_total;
  }

  json response;
  response["device_count"] = clients.size();
  response["reporting_count"] = reporting;
  if (reporting > 0) {
    response["min_offset_us"] = min_offset_us;
    response["max_offset_us"] = max_offset_us;
    if (skew_reporting > 0) {
      response["fleet_skew_us"] = max_sync_error_us - min_sync_error_us;
    } else {
      response["fleet_skew_us"] = nullptr;
    }
    response["mean_rtt_us"] = sum_rtt_us / reporting;
    response["min_rtt_us"] = min_rtt_us == std::numeric_limits<uint64_t>::max() ? 0 : min_rtt_us;
    response["max_rtt_us"] = max_rtt_us;
    response["slowest_device_board_id"] = slowest_id;
  } else {
    response["min_offset_us"] = nullptr;
    response["max_offset_us"] = nullptr;
    response["fleet_skew_us"] = nullptr;
    response["mean_rtt_us"] = nullptr;
    response["min_rtt_us"] = nullptr;
    response["max_rtt_us"] = nullptr;
    response["slowest_device_board_id"] = "";
  }
  response["total_next_beat_gap"] = total_next_beat_gap;
  response["total_intercore_drops"] = total_intercore_drops;
  response["total_time_sync_outliers"] = total_time_sync_outliers;
  response["thresholds"] = {{"skew_warn_us", qos_thresholds_.skew_warn_us},
                            {"skew_fail_us", qos_thresholds_.skew_fail_us}};

  // Server-side health pip — React just renders the verdict. Either
  // threshold crossing OR a non-zero drop counter shifts the colour:
  // those drop counters reflect bugs operators should look at even when
  // the fleet skew is fine.
  std::string health = "unknown";
  if (reporting > 0) {
    const uint64_t skew_us = (skew_reporting > 0 && max_sync_error_us > min_sync_error_us)
                                 ? static_cast<uint64_t>(max_sync_error_us - min_sync_error_us)
                                 : 0;
    if (skew_us >= qos_thresholds_.skew_fail_us || total_intercore_drops > 0 ||
        total_time_sync_outliers > 0) {
      health = "fail";
    } else if (skew_us >= qos_thresholds_.skew_warn_us) {
      health = "warn";
    } else {
      health = "ok";
    }
  }
  response["health"] = health;

  return init_resp(req->create_response(restinio::status_ok())).set_body(response.dump()).done();
}

APIHandler::req_status_t APIHandler::on_get_health(const req_handle_t &req, route_params_t params) {
  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(R"({"status":"ok"})")
      .done();
}

APIHandler::req_status_t APIHandler::on_preflight(const req_handle_t &req, route_params_t params) {
  return init_resp(req->create_response(restinio::status_ok())).done();
}

} // namespace beatled::server
