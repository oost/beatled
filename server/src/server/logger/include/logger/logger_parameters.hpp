#ifndef LOGGER_PARAMETERS_H
#define LOGGER_PARAMETERS_H

// #include <cstdint>
#include <string>

namespace server {

struct logger_parameters_t {
  std::size_t queue_size = 20;
};

} // namespace server

#endif // LOGGER_PARAMETERS_H