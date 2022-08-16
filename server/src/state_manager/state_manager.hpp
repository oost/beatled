#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <asio.hpp>
#include <memory>

struct precise_time_t {
  long tv_sec;
  long tv_nsec;
};

class StateManager {
public:
  StateManager();

  void update_tempo(float tempo, const precise_time_t &timeref);
  float get_tempo() const { return tempo_; }
  precise_time_t get_time_ref() const { return time_ref_; }
  void broadcast_tempo();

  // asio::strand<asio::any_io_executor> &strand();

private:
  float tempo_;
  precise_time_t time_ref_;
  // asio::strand<asio::any_io_executor> strand_;
};

#endif // STATE_MACHINE_H