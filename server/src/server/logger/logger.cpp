
#include "logger.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "date/date.h"

using namespace server;

std::string get_current_time() {
  std::time_t t =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::stringstream transTime;
  transTime << std::put_time(std::localtime(&t), "%FT%T%z");
  return transTime.str();
}

template <class Precision> std::string getISOCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  return date::format("%FT%TZ", date::floor<Precision>(now));
}

void Logger::log_message(const char *level, const char *message) {
  log_queue_.push_back(LogMessage{
      getISOCurrentTimestamp<std::chrono::milliseconds>(), level, message});

  while (log_queue_.size() > max_queue_size_) {
    log_queue_.pop_front();
  }
}

json Logger::log_tail() {
  json data;

  for (auto &msg : log_queue_) {
    data.push_back(msg);
  }

  return data;
}
