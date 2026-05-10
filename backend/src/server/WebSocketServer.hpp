#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <functional>
#include <deque>

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
using tcp       = net::ip::tcp;

namespace server {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
class WsSession;

// ---------------------------------------------------------------------------
// SessionRegistry — thread-safe registry of active WebSocket sessions
// ---------------------------------------------------------------------------

/// Maintains weak references to all live WsSession objects.
/// Allows broadcasting a message to every connected client.
class SessionRegistry {
public:
    void add(std::shared_ptr<WsSession> session);
    void remove(std::shared_ptr<WsSession> session);

    /// Send a text frame to every active session.
    void broadcast(const std::string& message);

private:
    std::mutex                                                     mutex_;
    std::set<std::weak_ptr<WsSession>,
             std::owner_less<std::weak_ptr<WsSession>>>           sessions_;
};

// ---------------------------------------------------------------------------
// WsSession — a single WebSocket client connection
// ---------------------------------------------------------------------------

/// Handles the full lifecycle of one client: HTTP handshake → WS upgrade →
/// receive loop → graceful close.  Uses a per-session message queue so that
/// concurrent broadcast() calls never race on the wire.
class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    explicit WsSession(tcp::socket            socket,
                       std::shared_ptr<SessionRegistry> registry,
                       std::string            static_root);

    /// Begin reading the initial HTTP request.
    void run();

    /// Queue a text message for delivery.  Thread-safe.
    void send(std::shared_ptr<std::string const> message);

private:
    ws::stream<beast::tcp_stream>            stream_;
    beast::flat_buffer                       buffer_;
    std::shared_ptr<SessionRegistry>         registry_;
    std::string                              static_root_;

    // Per-session outbound message queue.
    std::mutex                               queue_mutex_;
    std::deque<std::shared_ptr<std::string const>> queue_;

    // Used only while we're in the HTTP phase.
    http::request<http::string_body>         http_req_;

    void on_read_http(beast::error_code ec, std::size_t bytes);
    void do_accept_ws();
    void on_accept_ws(beast::error_code ec);
    void do_ws_read();
    void on_ws_read(beast::error_code ec, std::size_t bytes);
    void on_ws_write(beast::error_code ec, std::size_t bytes);
};

// ---------------------------------------------------------------------------
// WebSocketServer — accepts connections and owns the session registry
// ---------------------------------------------------------------------------

/// Listens on a single TCP port and upgrades connections to WebSocket or
/// serves static HTTP files from the given directory.
class WebSocketServer {
public:
    WebSocketServer(net::io_context& ioc,
                    uint16_t         port,
                    std::string      static_root);

    /// Begin accepting connections.  Must be called before ioc.run().
    void start();

    /// Broadcast a JSON string to all connected WebSocket clients.
    void broadcast(const std::string& message);

private:
    net::io_context&                 ioc_;
    tcp::acceptor                    acceptor_;
    std::string                      static_root_;
    std::shared_ptr<SessionRegistry> registry_;

    void do_accept();
};

} // namespace server
