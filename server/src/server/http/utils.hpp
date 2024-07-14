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

template <typename RESP> RESP init_resp_cors(RESP resp) {
  return init_resp<RESP>(std::forward<RESP>(resp))
      .append_header(restinio::http_field::access_control_allow_origin, "*")
      .append_header(restinio::http_field::access_control_allow_headers,
                     "Origin, X-Requested-With, Content-Type, Accept");
}

} // namespace beatled::server
#endif // HTTP_SERVER__UTILS_HPP