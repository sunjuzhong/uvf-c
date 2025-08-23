#!/usr/bin/env bash
set -euo pipefail
# Build wasm target
# Usage: scripts/build_wasm.sh <emsdk_root(optional)> <build_dir(optional)>

EMSDK_ROOT=${1:-""}
BUILD_DIR=${2:-"build-wasm"}

if [[ -n "$EMSDK_ROOT" ]]; then
  source "$EMSDK_ROOT/emsdk_env.sh"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Artifacts: $(pwd)/uvf.js and uvf.wasm"
