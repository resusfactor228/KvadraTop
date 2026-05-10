#!/usr/bin/env bash
# =============================================================================
# KvadraTop — Build Script
#
# Builds both the TypeScript frontend (via Vite) and the C++ backend (via CMake).
# Run from the repository root or any subdirectory.
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/backend/build"
FRONTEND_DIR="$ROOT/frontend"

echo "==> Building frontend..."
cd "$FRONTEND_DIR"
npm install
npm run build

echo ""
echo "==> Building backend..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"

echo ""
echo "============================================================"
echo "Build complete!"
echo ""
echo "Run the server:"
echo "  $BUILD_DIR/kvadra-top [port] [$FRONTEND_DIR/dist]"
echo ""
echo "Or use the convenience script:"
echo "  $ROOT/scripts/run.sh [port]"
echo "============================================================"
