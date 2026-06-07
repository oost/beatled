
#include <chrono>
#include <date/date.h>
#include <iomanip>
#include <spdlog/async.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <spdlog/spdlog.h>
#include <sstream>

#include "logger/logger.hpp"

using namespace beatled::server;

// Map the CLI's free-form log-level string to a spdlog enum value.
// Unrecognized values fall back to info with a warning so a typo doesn't
// silently leave the server logging less than the operator expected.
static spdlog::level::level_enum parse_log_level(const std::string &name, bool *recognized_out) {
  if (recognized_out)
    *recognized_out = true;
  if (name == "trace")
    return spdlog::level::trace;
  if (name == "debug")
    return spdlog::level::debug;
  if (name == "info")
    return spdlog::level::info;
  if (name == "warn" || name == "warning")
    return spdlog::level::warn;
  if (name == "err" || name == "error")
    return spdlog::level::err;
  if (name == "critical" || name == "crit")
    return spdlog::level::critical;
  if (name == "off")
    return spdlog::level::off;
  if (recognized_out)
    *recognized_out = false;
  return spdlog::level::info;
}

Logger::Logger(const Logger::parameters_t &logger_parameters) {
  spdlog::init_thread_pool(8192, 1);
  ringbuffer_ = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(1024);
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
  sinks.push_back(ringbuffer_);

  auto logger = std::make_shared<spdlog::async_logger>(
      "logger", std::begin(sinks), std::end(sinks), spdlog::thread_pool(),
      spdlog::async_overflow_policy::overrun_oldest);
  spdlog::set_default_logger(logger);
  spdlog::flush_every(std::chrono::seconds(1));

  bool recognized = true;
  auto level = parse_log_level(logger_parameters.log_level, &recognized);
  logger->set_level(level);
  if (!recognized) {
    SPDLOG_WARN("Unrecognized log level '{}'; falling back to 'info'. "
                "Valid values: trace, debug, info, warn, err, critical, off.",
                logger_parameters.log_level);
  } else {
    SPDLOG_INFO("Logger initialised at level '{}'", logger_parameters.log_level);
  }
}

json Logger::log_tail() {
  json data;

  for (auto &msg : ringbuffer_->last_formatted()) {
    data.push_back(msg);
  }

  return data;
}
