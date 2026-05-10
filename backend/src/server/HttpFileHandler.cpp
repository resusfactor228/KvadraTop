#include "HttpFileHandler.hpp"
#include <fstream>
#include <iterator>
#include <filesystem>

namespace server {

namespace http = boost::beast::http;

// ---------------------------------------------------------------------------
// MIME type mapping
// ---------------------------------------------------------------------------

std::string mime_type(std::string_view path) {
    const auto dot = path.rfind('.');
    if (dot == std::string_view::npos) return "application/octet-stream";

    const auto ext = path.substr(dot);
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".css")  return "text/css; charset=utf-8";
    if (ext == ".js")   return "text/javascript; charset=utf-8";
    if (ext == ".mjs")  return "text/javascript; charset=utf-8";
    if (ext == ".ts")   return "text/typescript";
    if (ext == ".json") return "application/json";
    if (ext == ".map")  return "application/json";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg")  return "image/jpeg";
    if (ext == ".jpeg") return "image/jpeg";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2")return "font/woff2";
    if (ext == ".ttf")  return "font/ttf";
    return "application/octet-stream";
}

// ---------------------------------------------------------------------------
// File reading
// ---------------------------------------------------------------------------

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// ---------------------------------------------------------------------------
// HTTP response builder
// ---------------------------------------------------------------------------

http::response<http::string_body>
serve_file(const http::request<http::string_body>& req,
           const std::string& static_root) {
    // Normalise the URL path.
    std::string target(req.target());

    // Strip query string.
    const auto qmark = target.find('?');
    if (qmark != std::string::npos) target = target.substr(0, qmark);

    // Map "/" and "/index.html" to index.html.
    if (target == "/" || target.empty()) {
        target = "index.html";
    } else {
        // Strip leading slash.
        if (target.front() == '/') target = target.substr(1);
    }

    // Security: reject paths that try to escape the root.
    const std::filesystem::path root(static_root);
    const std::filesystem::path requested = root / target;
    // Resolve without following symlinks to detect traversal attempts.
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(requested, ec);
    const auto canon_root = std::filesystem::weakly_canonical(root, ec);
    const std::string canonical_str = canonical.string();
    const std::string root_str      = canon_root.string();
    if (canonical_str.rfind(root_str, 0) != 0) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.body() = "403 Forbidden\n";
        res.prepare_payload();
        return res;
    }

    const std::string full_path = static_root + "/" + target;
    const std::string body      = read_file(full_path);

    if (body.empty() && !std::filesystem::exists(full_path)) {
        // 404
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/html; charset=utf-8");
        res.body() = "<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                     "<p>The requested resource was not found.</p></body></html>\n";
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, mime_type(full_path));
    res.body() = body;
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
}

} // namespace server
