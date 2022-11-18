#ifndef LOGGER__RING_BUFFER_SINK_HPP
#define LOGGER__RING_BUFFER_SINK_HPP

#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

#include "logger/logger_parameters.hpp"

using json = nlohmann::json;

struct LogMessage {
  std::string time;
  std::string level;
  std::string message;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogMessage, time, level, message)

template <typename Mutex>
class ring_buffer_sink : public spdlog::sinks::base_sink<Mutex> {
protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {

    // log_msg is a struct containing the log entry info like level, timestamp,
    // thread id etc. msg.raw contains pre formatted log

    // If needed (very likely but not mandatory), the sink formats the message
    // before sending it to its final destination:
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    std::cout << fmt::to_string(formatted);
  }

  void flush_() override { std::cout << std::flush; }

  std::deque<LogMessage> log_queue_;
  std::size_t max_queue_size_;
};

using ring_buffer_sink_mt = ring_buffer_sink<std::mutex>;
using ring_buffer_sink_st = ring_buffer_sink<spdlog::details::null_mutex>;

#endif // LOGGER__RING_BUFFER_SINK_HPP
