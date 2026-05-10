/**
 * KvadraTop - PC Resource Monitor
 *
 * Backend entry point.  Starts the WebSocket/HTTP server, then runs a
 * periodic metrics collection loop that broadcasts JSON to all clients.
 *
 * Usage:
 *   kvadra-top [port] [frontend-path]
 *
 *   port           TCP port to listen on          (default: 8080)
 *   frontend-path  Directory containing the built frontend
 *                                                  (default: ../frontend/dist)
 *
 * The server accepts both plain HTTP GET requests (for static frontend files)
 * and WebSocket upgrade requests on the same port.  A new JSON snapshot is
 * broadcast to every connected WebSocket client every second.
 */
#include "MetricsCollector.hpp"
#include "Serializer.hpp"
#include "server/WebSocketServer.hpp"

#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

namespace net = boost::asio;

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int /*sig*/) {
    g_running = false;
}

} // namespace

int main(int argc, char* argv[]) {
    const uint16_t    port         = (argc > 1) ? static_cast<uint16_t>(std::stoul(argv[1])) : 8080;
    const std::string frontend_dir = (argc > 2) ? argv[2] : "../frontend/dist";

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "[kvadra-top] Starting on port "       << port         << "\n"
              << "[kvadra-top] Serving frontend from: " << frontend_dir << "\n";

    net::io_context ioc;

    // Create and start the WebSocket / HTTP server.
    server::WebSocketServer ws_server(ioc, port, frontend_dir);
    ws_server.start();

    // Run the Asio event loop in a background thread so the main thread is
    // free to run the blocking metrics-collection loop.
    std::thread io_thread([&ioc]() {
        try {
            ioc.run();
        } catch (const std::exception& ex) {
            std::cerr << "[kvadra-top] io_context error: " << ex.what() << "\n";
        }
    });

    MetricsCollector collector;

    // Main loop: collect and broadcast metrics every second.
    while (g_running) {
        try {
            const auto snap = collector.collect();
            const auto json = serialize_snapshot(snap);
            ws_server.broadcast(json);
        } catch (const std::exception& ex) {
            std::cerr << "[kvadra-top] Metrics error: " << ex.what() << "\n";
        }

        // Sleep in small increments so the signal handler can wake us up.
        for (int i = 0; i < 10 && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[kvadra-top] Shutting down...\n";
    ioc.stop();
    io_thread.join();

    return 0;
}
