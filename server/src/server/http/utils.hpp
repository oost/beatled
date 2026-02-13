#ifndef HTTP_SERVER__UTILS_HPP
#define HTTP_SERVER__UTILS_HPP

#include <utility>

#include <restinio/core.hpp>

namespace beatled::server {
template <typename RESP> RESP init_resp(RESP resp) {
  resp.append_header(restinio::http_field::server, "Beatled server /v.0.2");
  resp.append_header_date_field();

  return resp;
}

} // namespace beatled::server
#endif // HTTP_SERVER__UTILS_HPP