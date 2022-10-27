#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <asio.hpp>
#include <memory>
#include <time.h>

struct sntp_time_t {
  uint32_t tv_sec;
  uint32_t tv_nsec;
};

class StateManager {
public:
  StateManager(asio::io_context &io_context);

  void update_tempo(float tempo, const sntp_time_t &timeref);
  float get_tempo() const { return tempo_; }
  sntp_time_t get_time_ref() const { return time_ref_; }
  void broadcast_tempo();
  void
  post_tempo(const std::function<void(float, uint32_t, uint32_t)> &post_cb);
  // asio::strand<asio::any_io_executor> &strand();

private:
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;
  float tempo_;
  sntp_time_t time_ref_;
  asio::strand<asio::io_context::executor_type> strand_;
};

#endif // STATE_MACHINE_H