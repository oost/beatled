
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
  logger->set_level(logger_parameters.verbose ? spdlog::level::debug
                                               : spdlog::level::info);
}

json Logger::log_tail() {
  json data;

  for (auto &msg : ringbuffer_->last_formatted()) {
    data.push_back(msg);
  }

  return data;
}
