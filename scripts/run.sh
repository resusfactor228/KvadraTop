#!/usr/bin/env bash
# =============================================================================
# KvadraTop — Run Script
#
# Usage: ./scripts/run.sh [port]
#   port   TCP port for the server (default: 8080)
#
# The server serves the built frontend from frontend/dist/ and accepts
# WebSocket connections from the browser on the same port.
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${1:-8080}"
BINARY="$ROOT/backend/build/kvadra-top"
FRONTEND_DIST="$ROOT/frontend/dist"

if [[ ! -x "$BINARY" ]]; then
    echo "Error: binary not found at $BINARY"
    echo "Run ./scripts/build.sh first."
    exit 1
fi

if [[ ! -d "$FRONTEND_DIST" ]]; then
    echo "Error: frontend dist not found at $FRONTEND_DIST"
    echo "Run ./scripts/build.sh first."
    exit 1
fi

echo "Starting KvadraTop on http://localhost:$PORT"
exec "$BINARY" "$PORT" "$FRONTEND_DIST"
