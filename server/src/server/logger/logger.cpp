
#include <chrono>
#include <date/date.h>
#include <iomanip>
#include <spdlog/async.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <spdlog/spdlog.h>
#include <sstream>

#include "logger/logger.hpp"

using namespace server;

Logger::Logger(const logger_parameters_t &logger_parameters)
// : max_queue_size_{logger_parameters.queue_size}
{
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
}

// std::string get_current_time() {
//   std::time_t t =
//       std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//   std::stringstream transTime;
//   transTime << std::put_time(std::localtime(&t), "%FT%T%z");
//   return transTime.str();
// }

// template <class Precision> std::string getISOCurrentTimestamp() {
//   auto now = std::chrono::system_clock::now();
//   return date::format("%FT%TZ", date::floor<Precision>(now));
// }

// void Logger::log_message(const char *level, const char *message) {
//   log_queue_.push_back(LogMessage{
//       getISOCurrentTimestamp<std::chrono::milliseconds>(), level, message});

//   while (log_queue_.size() > max_queue_size_) {
//     log_queue_.pop_front();
//   }
// }

json Logger::log_tail() {
  json data;

  for (auto &msg : ringbuffer_->last_formatted()) {
    data.push_back(msg);
  }

  return data;
}
