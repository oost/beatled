#include <restinio/all.hpp>

#include "./file_handler.hpp"

using namespace beatled::server;

restinio::request_handling_status_t
FileHandler::serve_file(const restinio::request_handle_t &req,
                        std::string_view request_path,
                        std::string_view extension) {
  if (std::string::npos == request_path.find("..")) {
    // A nice path.

    const auto file_path = root_dir_ / request_path.substr(1);

    try {
      auto sf = restinio::sendfile(file_path);
      auto modified_at =
          restinio::make_date_field_value(sf.meta().last_modified_at());

      auto expires_at = restinio::make_date_field_value(
          std::chrono::system_clock::now() + std::chrono::hours(24 * 7));

      return req->create_response()
          .append_header(restinio::http_field::server, "RESTinio")
          .append_header_date_field()
          .append_header(restinio::http_field::last_modified,
                         std::move(modified_at))
          .append_header(restinio::http_field::expires, std::move(expires_at))
          .append_header(restinio::http_field::content_type,
                         content_type_by_file_extention(extension))
          .set_body(std::move(sf))
          .done();
    } catch (const std::exception &) {
      return req->create_response(restinio::status_not_found())
          .append_header_date_field()
          .connection_close()
          .done();
    }
  } else {
    // Bad path.
    return req->create_response(restinio::status_forbidden())
        .append_header_date_field()
        .connection_close()
        .done();
  }
}

FileHandler::FileHandler(const std::string &root_dir) : root_dir_{root_dir} {}

restinio::request_handling_status_t
FileHandler::on_file_request(const restinio::request_handle_t &req,
                             restinio::router::route_params_t params) {

  return serve_file(req, req->header().path(), params["ext"]);
}

// GET request to homepage.
restinio::request_handling_status_t
FileHandler::on_root_request(const restinio::request_handle_t &req,
                             restinio::router::route_params_t) {
  return serve_file(req, "/index.html", "html");

  // return req->create_response(restinio::status_temporary_redirect())
  //     .append_header(restinio::http_field::location, "/index.html")
  //     .append_header_date_field()
  //     .connection_close()
  //     .done();
}
