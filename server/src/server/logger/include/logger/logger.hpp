#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <deque>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <string>

using json = nlohmann::json;

namespace beatled::server {

class Logger {
public:
  struct parameters_t {
    std::size_t queue_size = 20;
  };

  Logger(const parameters_t &logger_parameters);

  json log_tail();

private:
  std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_;
};

} // namespace beatled::server
#endif // SERVER_LOG_H