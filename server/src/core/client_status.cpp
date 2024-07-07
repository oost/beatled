#include "core/client_status.hpp"

namespace beatled::core {

void to_json(json &j, const ClientStatus::board_id_t &board_id) {
  j = json{{"board_id", fmt::format("{}", board_id)}};
}
} // namespace beatled::core
