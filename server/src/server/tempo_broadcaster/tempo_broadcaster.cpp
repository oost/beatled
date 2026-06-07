#include <asio.hpp>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

#include "core/state_manager.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp/udp_buffer.hpp"

namespace beatled::server {

using asio::ip::udp;
using beatled::core::StateManager;
using beatled::core::tempo_ref_t;

TempoBroadcaster::TempoBroadcaster(const std::string &id, asio::io_context &io_context,
                                   std::chrono::nanoseconds program_refresh_period,
                                   const parameters_t &broadcasting_server_parameters,
                                   StateManager &state_manager)
    : ServiceControllerInterface{id}, state_manager_{state_manager}, io_context_(io_context),
      program_refresh_timer_(asio::high_resolution_timer(io_context_, program_refresh_period)),
      program_refresh_period_(program_refresh_period),
      socket_(std::make_shared<asio::ip::udp::socket>(io_context)),
      broadcast_endpoint_{
          udp::endpoint(asio::ip::make_address_v4(broadcasting_server_parameters.address),
                        broadcasting_server_parameters.port)},
      broadcasting_server_parameters_{broadcasting_server_parameters},
      strand_{asio::make_strand(io_context)} {
  SPDLOG_INFO("Creating {}", name());

  socket_->open(udp::v4());
  if (!socket_->is_open()) {
    throw std::runtime_error("Can't open UDP socket");
  }

  socket_->set_option(udp::socket::reuse_address(true));
  if (broadcasting_server_parameters_.mode != BroadcastMode::Unicast) {
    socket_->set_option(asio::socket_base::broadcast(true));
  }

  const char *mode_name =
      broadcasting_server_parameters_.mode == BroadcastMode::Unicast
          ? "unicast"
          : (broadcasting_server_parameters_.mode == BroadcastMode::Subnet ? "subnet-broadcast"
                                                                           : "limited-broadcast");
  SPDLOG_INFO("{} mode={} bind={} dst={}", name(), mode_name,
              fmt::streamed(socket_->local_endpoint()), fmt::streamed(broadcast_endpoint_));

  // Hook program changes for instant push.
  state_manager_.register_program_change_cb([this](uint16_t /*pid*/) {
    if (is_running()) {
      push_program_now();
    }
  });
}

TempoBroadcaster::~TempoBroadcaster() {}

void TempoBroadcaster::broadcast_next_beat(uint64_t next_beat_time_ref, uint32_t beat_count) {
  if (!is_running()) {
    SPDLOG_INFO("TempoBroadcaster is not on. Dropping next beat broadcast.");
    return;
  }

  asio::post(strand_, [this, next_beat_time_ref, beat_count]() {
    uint16_t seq = next_beat_seq_.fetch_add(1, std::memory_order_relaxed);
    auto buffer = std::make_unique<NextBeatBuffer>(next_beat_time_ref, beat_count, seq);
    // Per-beat traffic — same reasoning as application.cpp.
    SPDLOG_DEBUG("{} next_beat seq={} t={} count={}", name(), seq, next_beat_time_ref, beat_count);
    dispatch(std::move(buffer), /*compensate_owd=*/true, next_beat_time_ref);
  });
}

void TempoBroadcaster::broadcast_beat(uint64_t beat_time_ref, uint32_t beat_count) {
  if (!is_running()) {
    SPDLOG_INFO("TempoBroadcaster is not on. Dropping beat broadcast.");
    return;
  }

  asio::post(strand_, [this, beat_time_ref, beat_count]() {
    uint16_t seq = beat_seq_.fetch_add(1, std::memory_order_relaxed);
    auto buffer = std::make_unique<BeatBuffer>(beat_time_ref, beat_count, seq);
    dispatch(std::move(buffer), /*compensate_owd=*/true, beat_time_ref);
  });
}

void TempoBroadcaster::push_program_now() {
  if (!is_running()) {
    return;
  }
  asio::post(strand_, [this]() {
    uint16_t pid = state_manager_.get_program_id();
    uint16_t seq = program_seq_.fetch_add(1, std::memory_order_relaxed);
    SPDLOG_INFO("{} program push seq={} pid={}", name(), seq, pid);

    // Immediate send.
    dispatch(std::make_unique<ProgramPushBuffer>(pid, seq),
             /*compensate_owd=*/false, 0);

    // Retry 50 ms later with the same seq so controllers that already
    // applied the first packet treat the retry as an idempotent no-op
    // (command.c drops `delta < 0` and skips the intercore notification
    // when `registry.program_id` is unchanged). One extra 5-byte UDP
    // packet per registered client per program change — cheap insurance
    // against Wi-Fi loss bursts that flatten the on-change push.
    auto retry =
        std::make_shared<asio::high_resolution_timer>(strand_, std::chrono::milliseconds(50));
    retry->async_wait(
        asio::bind_executor(strand_, [this, pid, seq, retry](const asio::error_code &ec) {
          if (ec || !is_running()) {
            return;
          }
          dispatch(std::make_unique<ProgramPushBuffer>(pid, seq),
                   /*compensate_owd=*/false, 0);
        }));
  });
}

void TempoBroadcaster::schedule_program_refresh() {
  program_refresh_timer_.expires_after(program_refresh_period_);
  program_refresh_timer_.async_wait(
      asio::bind_executor(strand_, [this](const asio::error_code &error) {
        if (error == asio::error::operation_aborted) {
          return;
        }
        if (error) {
          SPDLOG_ERROR("Program refresh timer error: {}", error.message());
          return;
        }
        push_program_now();
        if (is_running()) {
          schedule_program_refresh();
        }
      }));
}

void TempoBroadcaster::dispatch(DataBuffer::Ptr response_buffer, bool compensate_owd,
                                uint64_t base_time_for_compensation) {
  if (broadcasting_server_parameters_.mode != BroadcastMode::Unicast) {
    // Broadcast mode: one packet, no per-client compensation possible.
    auto shared = std::shared_ptr<DataBuffer>(std::move(response_buffer));
    send_to_endpoint(std::move(shared), broadcast_endpoint_);
    return;
  }

  // Unicast mode. Snapshot the client list (also prunes stale entries) and
  // send one frame per registered client. For NEXT_BEAT/BEAT we rewrite the
  // time field per-recipient to compensate measured one-way delay.
  auto clients = state_manager_.get_clients();
  if (clients.empty()) {
    return;
  }

  uint8_t type = response_buffer->type();
  bool can_compensate =
      compensate_owd && (type == BEATLED_MESSAGE_NEXT_BEAT || type == BEATLED_MESSAGE_BEAT);

  for (const auto &cs : clients) {
    if (cs->endpoint.port() == 0) {
      continue; // never observed a real endpoint
    }

    if (!can_compensate || cs->owd_us == 0) {
      // No per-client adjustment — share the same bytes.
      auto shared = std::shared_ptr<DataBuffer>(new auto(*response_buffer));
      send_to_endpoint(std::move(shared), cs->endpoint);
      continue;
    }

    // Build a per-client copy with the time field shifted back by OWD so
    // when the packet *arrives* at the controller, the embedded
    // next_beat_time_ref equals the server's intended hit time.
    uint64_t adjusted =
        base_time_for_compensation > cs->owd_us ? base_time_for_compensation - cs->owd_us : 0;
    DataBuffer::Ptr per_client;
    if (type == BEATLED_MESSAGE_NEXT_BEAT) {
      const auto *src =
          reinterpret_cast<const beatled_message_next_beat_t *>(response_buffer->data().data());
      per_client =
          std::make_unique<NextBeatBuffer>(adjusted, ntohl(src->beat_count), ntohs(src->seq));
    } else {
      const auto *src =
          reinterpret_cast<const beatled_message_beat_t *>(response_buffer->data().data());
      per_client = std::make_unique<BeatBuffer>(adjusted, ntohl(src->beat_count), ntohs(src->seq));
    }
    auto shared = std::shared_ptr<DataBuffer>(std::move(per_client));
    send_to_endpoint(std::move(shared), cs->endpoint);
  }
}

void TempoBroadcaster::send_to_endpoint(std::shared_ptr<DataBuffer> buffer,
                                        const asio::ip::udp::endpoint &endpoint) {
  socket_->async_send_to(
      asio::buffer(buffer->data(), buffer->size()), endpoint,
      asio::bind_executor(strand_, [buffer, endpoint](std::error_code ec, std::size_t /*sent*/) {
        if (ec) {
          SPDLOG_ERROR("Send to {} failed: {}", fmt::streamed(endpoint), ec.message());
        }
      }));
}

void TempoBroadcaster::start_sync() {
  schedule_program_refresh();
}

void TempoBroadcaster::stop_sync() {
  program_refresh_timer_.cancel();
  socket_->cancel();
}

} // namespace beatled::server
