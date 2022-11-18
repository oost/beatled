#ifndef BEATLED_CONFIG__CONFIG_H
#define BEATLED_CONFIG__CONFIG_H

#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <map>
#include <memory>
#include <thread>

#include "server/server.hpp"

class BeatledConfig {
public:
  BeatledConfig(int argc, const char *argv[]);

  server::server_parameters_t server_parameters() const;
  bool help() const { return m_help; }

private:
  bool m_help{false};
  std::string m_address{"localhost"};
  bool m_start_http_server = false;
  bool m_start_udp_server = false;
  bool m_start_broadcaster = false;
  std::uint16_t m_http_port{8080};
  std::uint16_t m_udp_port{9090};
  std::string m_broadcasting_address{"192.168.86.255"};
  std::uint16_t m_broadcasting_port{8765};
  std::size_t m_pool_size{2};
  std::string m_root_dir{"."};
};

#endif // BEATLED_CONFIG__CONFIG_H