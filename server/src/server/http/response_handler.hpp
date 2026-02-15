#ifndef HTTP_SERVER__RESPONSE_HANDLER_HPP
#define HTTP_SERVER__RESPONSE_HANDLER_HPP

#include <restinio/core.hpp>
#include <string>
#include <string_view>

namespace beatled::server {
class ResponseHandler {
public:
protected:
  static const char *content_type_by_file_extention(const std::string_view ext);

  void set_cors_origin(const std::string &origin) { cors_origin_ = origin; }

  template <typename RESP> RESP init_resp(RESP resp) {
    resp.append_header(restinio::http_field::server, "Beatled server /v.0.1");
    resp.append_header_date_field();

    if (!csp_header_.empty()) {
      resp.append_header("Content-Security-Policy", csp_header_);
    }

    return resp;
  }

  void build_csp(const std::string &cors_origin) {
    std::string connect_src = "'self'";
    if (!cors_origin.empty()) {
      connect_src += " " + cors_origin;
    }
    connect_src += " https://beatled.local:8443 https://beatled.test:8443"
                   " https://raspberrypi1.local:8443 https://localhost:8443";
    csp_header_ = "default-src 'self'; script-src 'self'; "
                  "style-src 'self' 'unsafe-inline'; "
                  "font-src 'self' data:; "
                  "connect-src " +
                  connect_src +
                  "; "
                  "img-src 'self' data:";
  }

private:
  std::string cors_origin_;
  std::string csp_header_;
};
} // namespace beatled::server

#endif // HTTP_SERVER__RESPONSE_HANDLER_HPP