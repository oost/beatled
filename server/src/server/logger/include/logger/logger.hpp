#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <deque>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <string>

#include "logger_parameters.hpp"

using json = nlohmann::json;

namespace server {

struct LogMessage {
  std::string time;
  std::string level;
  std::string message;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogMessage, time, level, message)

class Logger {
public:
  Logger(const logger_parameters_t &logger_parameters);

  json log_tail();

private:
  std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_;
};

} // namespace server

#endif // SERVER_LOG_H