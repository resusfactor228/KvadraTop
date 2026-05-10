#pragma once
#include <boost/beast/http.hpp>
#include <string>
#include <string_view>

namespace server {

namespace http = boost::beast::http;

/// Returns the MIME type string for a file path based on its extension.
std::string mime_type(std::string_view path);

/// Reads the entire contents of a file into a string.
/// Returns an empty string if the file cannot be opened.
std::string read_file(const std::string& path);

/// Builds an HTTP response that serves a static file from static_root.
/// @param req         The incoming HTTP request (used for keep-alive and version).
/// @param static_root Filesystem directory that is the root of the served files.
http::response<http::string_body>
serve_file(const http::request<http::string_body>& req,
           const std::string& static_root);

} // namespace server
