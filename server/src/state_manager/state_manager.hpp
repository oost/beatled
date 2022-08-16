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
  static void initialize() {
    if (instance_ == nullptr) {
      instance_ = new (std::nothrow)
          StateManager; // Threads are insecure without locking, and multiple
                        // instances are created when threads are concurrent
    }
  }

  static StateManager *get_instance() { return instance_; }

  void update_tempo(float tempo, const precise_time_t &timeref);
  float get_tempo() const { return tempo_; }
  precise_time_t get_time_ref() const { return time_ref_; }
  void broadcast_tempo();

  asio::strand<asio::any_io_executor> &strand();

  StateManager() {
    strand_ = std::move(asio::make_strand(asio::system_executor()));
  }

private:
  ~StateManager() = default;
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;

  // Unique singleton object pointer
  static StateManager *instance_;

  float tempo_;
  precise_time_t time_ref_;
  asio::strand<asio::any_io_executor> strand_;

  // Note: Scott Meyers mentions in his Effective Modern
  //       C++ book, that deleted functions should generally
  //       be public as it results in better error messages
  //       due to the compilers behavior to check accessibility
  //       before deleted status
};

#endif // STATE_MACHINE_H