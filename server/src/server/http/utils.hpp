#ifndef HTTP_SERVER__UTILS_HPP
#define HTTP_SERVER__UTILS_HPP

#include <utility>

#include <restinio/core.hpp>
#include <spdlog/spdlog.h>

namespace beatled::server {
template <typename RESP> RESP init_resp(RESP resp) {
  resp.append_header(restinio::http_field::server, "Beatled server /v.0.2");
  resp.append_header_date_field();

  // Log server errors (5xx status codes)
  if (resp.header().status_code() >= 500) {
    SPDLOG_ERROR("HTTP {} returned", resp.header().status_code());
  }

  return resp;
}

} // namespace beatled::server
#endif // HTTP_SERVER__UTILS_HPP