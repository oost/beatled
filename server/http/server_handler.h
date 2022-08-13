#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H

// #include <iostream>
// #include <map>
#include <restinio/all.hpp>
#include <fmt/format.h>
#include <json.hpp>

#include "file_extensions.h"

using json = nlohmann::json;


namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

auto server_handler(const std::string &root_dir)
{
	auto router = std::make_unique<router_t>();

	std::string server_root_dir;

	if (root_dir.empty())
	{
		server_root_dir = "./";
	}
	else if (root_dir.back() != '/' && root_dir.back() != '\\')
	{
		server_root_dir = root_dir + '/';
	}
	else
	{
		server_root_dir = root_dir;
	}

	router->http_get(
			"/api/status",
			[](auto req, auto)
			{
				// create an empty structure (null)
				json j;

				// add a number that is stored as double (note the implicit conversion of j to an object)
				j["message"] = "It's all good!";

				init_resp(req->create_response())
						.append_header(restinio::http_field::content_type, "text/json; charset=utf-8")
						.set_body(j.dump())
						.done();

				return restinio::request_accepted();
			});

	// GET request to homepage.
	router->http_get(
			R"(/:path(.*)\.:ext(.*))",
			restinio::path2regex::options_t{}.strict(true),
			[server_root_dir](auto req, auto params)
			{
				auto path = req->header().path();

				if (std::string::npos == path.find(".."))
				{
					// A nice path.

					const auto file_path =
							server_root_dir +
							std::string{path.data(), path.size()};

					try
					{
						auto sf = restinio::sendfile(file_path);
						auto modified_at =
								restinio::make_date_field_value(sf.meta().last_modified_at());

						auto expires_at =
								restinio::make_date_field_value(
										std::chrono::system_clock::now() +
										std::chrono::hours(24 * 7));

						return req->create_response()
								.append_header(
										restinio::http_field::server,
										"RESTinio")
								.append_header_date_field()
								.append_header(
										restinio::http_field::last_modified,
										std::move(modified_at))
								.append_header(
										restinio::http_field::expires,
										std::move(expires_at))
								.append_header(
										restinio::http_field::content_type,
										content_type_by_file_extention(params["ext"]))
								.set_body(std::move(sf))
								.done();
					}
					catch (const std::exception &)
					{
						return req->create_response(restinio::status_not_found())
								.append_header_date_field()
								.connection_close()
								.done();
					}
				}
				else
				{
					// Bad path.
					return req->create_response(restinio::status_forbidden())
							.append_header_date_field()
							.connection_close()
							.done();
				}
			});

	// GET request to homepage.
	router->http_get("/",[](auto req, auto)
			{
					return req->create_response(restinio::status_temporary_redirect())
  						.append_header(restinio::http_field::location, "/index.html")
							.append_header_date_field()
							.connection_close()
							.done();
			}
	);

	router->non_matched_request_handler(
			[](auto req)
			{
				if (restinio::http_method_get() == req->header().method())
					return req->create_response(restinio::status_not_found())
							.append_header_date_field()
							.connection_close()
							.done();

				return req->create_response(restinio::status_not_implemented())
						.append_header_date_field()
						.connection_close()
						.done();
			});

	return router;
}


#endif
