#ifndef HTTP_SERVER__RESPONSE_HANDLER_HPP
#define HTTP_SERVER__RESPONSE_HANDLER_HPP

#include <restinio/all.hpp>
#include <string_view>

namespace beatled::server {
class ResponseHandler {
public:
protected:
  static const char *content_type_by_file_extention(const std::string_view ext);

  template <typename RESP> RESP init_resp(RESP resp) {
    resp.append_header(restinio::http_field::server, "Beatled server /v.0.1");
    resp.append_header_date_field();

    return resp;
  }
};
} // namespace beatled::server

#endif // HTTP_SERVER__RESPONSE_HANDLER_HPP