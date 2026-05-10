#include "WebSocketServer.hpp"
#include "HttpFileHandler.hpp"
#include <iostream>

namespace server {

// ===========================================================================
// SessionRegistry
// ===========================================================================

void SessionRegistry::add(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

void SessionRegistry::remove(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}

void SessionRegistry::broadcast(const std::string& message) {
    auto msg = std::make_shared<std::string const>(message);

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (auto sp = it->lock()) {
            sp->send(msg);
            ++it;
        } else {
            it = sessions_.erase(it);
        }
    }
}

// ===========================================================================
// WsSession
// ===========================================================================

WsSession::WsSession(tcp::socket              socket,
                     std::shared_ptr<SessionRegistry> registry,
                     std::string              static_root)
    : stream_(std::move(socket))
    , registry_(std::move(registry))
    , static_root_(std::move(static_root))
{
}

void WsSession::run() {
    // Give the stream a 30-second timeout for the initial HTTP phase.
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Read the HTTP request.
    auto self = shared_from_this();
    http::async_read(
        beast::get_lowest_layer(stream_),
        buffer_,
        http_req_,
        [self](beast::error_code ec, std::size_t bytes) {
            self->on_read_http(ec, bytes);
        }
    );
}

void WsSession::on_read_http(beast::error_code ec, std::size_t /*bytes*/) {
    if (ec) {
        if (ec != beast::error::timeout) {
            std::cerr << "[ws] HTTP read error: " << ec.message() << "\n";
        }
        return;
    }

    // Check whether this is a WebSocket upgrade request.
    if (ws::is_upgrade(http_req_)) {
        do_accept_ws();
    } else {
        // Serve a static file over plain HTTP.
        auto resp = serve_file(http_req_, static_root_);
        auto resp_ptr = std::make_shared<http::response<http::string_body>>(std::move(resp));

        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        auto self = shared_from_this();
        http::async_write(
            beast::get_lowest_layer(stream_),
            *resp_ptr,
            [self, resp_ptr](beast::error_code wec, std::size_t) {
                if (wec) {
                    std::cerr << "[ws] HTTP write error: " << wec.message() << "\n";
                }
                // Close after response unless keep-alive was negotiated.
                beast::get_lowest_layer(self->stream_).close();
            }
        );
    }
}

void WsSession::do_accept_ws() {
    // Remove timeout — WebSocket connections can be long-lived.
    beast::get_lowest_layer(stream_).expires_never();

    // Set recommended server-side options.
    stream_.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));
    stream_.set_option(ws::stream_base::decorator([](ws::response_type& res) {
        res.set(http::field::server, "KvadraTop/1.0");
    }));

    auto self = shared_from_this();
    stream_.async_accept(
        http_req_,
        [self](beast::error_code ec) {
            self->on_accept_ws(ec);
        }
    );
}

void WsSession::on_accept_ws(beast::error_code ec) {
    if (ec) {
        std::cerr << "[ws] accept error: " << ec.message() << "\n";
        return;
    }

    registry_->add(shared_from_this());
    do_ws_read();
}

void WsSession::do_ws_read() {
    buffer_.consume(buffer_.size());
    auto self = shared_from_this();
    stream_.async_read(
        buffer_,
        [self](beast::error_code ec, std::size_t bytes) {
            self->on_ws_read(ec, bytes);
        }
    );
}

void WsSession::on_ws_read(beast::error_code ec, std::size_t /*bytes*/) {
    if (ec == ws::error::closed) {
        registry_->remove(shared_from_this());
        return;
    }
    if (ec) {
        registry_->remove(shared_from_this());
        return;
    }
    // We don't need to process incoming messages for a monitor,
    // but we must keep reading to detect disconnection.
    do_ws_read();
}

void WsSession::send(std::shared_ptr<std::string const> message) {
    // Post to the session's strand / executor so writes are serialised.
    net::post(
        stream_.get_executor(),
        [self = shared_from_this(), message = std::move(message)]() mutable {
            bool was_empty = false;
            {
                std::lock_guard<std::mutex> lk(self->queue_mutex_);
                was_empty = self->queue_.empty();
                self->queue_.push_back(std::move(message));
            }
            // If the queue was empty we must kick off a new write chain.
            if (was_empty) {
                std::shared_ptr<std::string const> front;
                {
                    std::lock_guard<std::mutex> lk(self->queue_mutex_);
                    if (!self->queue_.empty()) front = self->queue_.front();
                }
                if (front) {
                    self->stream_.async_write(
                        net::buffer(*front),
                        [self](beast::error_code ec, std::size_t bytes) {
                            self->on_ws_write(ec, bytes);
                        }
                    );
                }
            }
        }
    );
}

void WsSession::on_ws_write(beast::error_code ec, std::size_t /*bytes*/) {
    if (ec) {
        registry_->remove(shared_from_this());
        return;
    }

    std::shared_ptr<std::string const> next;
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        queue_.pop_front();
        if (!queue_.empty()) next = queue_.front();
    }

    if (next) {
        stream_.async_write(
            net::buffer(*next),
            [self = shared_from_this()](beast::error_code wec, std::size_t bytes) {
                self->on_ws_write(wec, bytes);
            }
        );
    }
}

// ===========================================================================
// WebSocketServer
// ===========================================================================

WebSocketServer::WebSocketServer(net::io_context& ioc,
                                 uint16_t         port,
                                 std::string      static_root)
    : ioc_(ioc)
    , acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    , static_root_(std::move(static_root))
    , registry_(std::make_shared<SessionRegistry>())
{
    acceptor_.set_option(net::socket_base::reuse_address(true));
}

void WebSocketServer::start() {
    do_accept();
}

void WebSocketServer::broadcast(const std::string& message) {
    registry_->broadcast(message);
}

void WebSocketServer::do_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<WsSession>(
                    std::move(socket), registry_, static_root_);
                session->run();
            } else {
                std::cerr << "[ws] accept error: " << ec.message() << "\n";
            }
            // Keep accepting more connections.
            do_accept();
        }
    );
}

} // namespace server
