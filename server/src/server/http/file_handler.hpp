#ifndef HTTP_SERVER__FILE_HANDLER_HPP
#define HTTP_SERVER__FILE_HANDLER_HPP

#include <filesystem>
#include <restinio/all.hpp>
#include <string>

namespace server {
class FileHandler {
public:
  FileHandler(const std::string &root_dir);

  restinio::request_handling_status_t
  on_file_request(const restinio::request_handle_t &req,
                  restinio::router::route_params_t);

  restinio::request_handling_status_t
  on_root_request(const restinio::request_handle_t &req,
                  restinio::router::route_params_t);

private:
  const std::filesystem::path root_dir_;
  restinio::request_handling_status_t
  serve_file(const restinio::request_handle_t &req,
             std::string_view request_path, std::string_view extension);
};
}; // namespace server

#endif // HTTP_SERVER__FILE_HANDLER_HPP