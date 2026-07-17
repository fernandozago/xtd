#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
BIN_DIR="$SCRIPT_DIR/bin"
RESULTS_DIR="$SCRIPT_DIR/results"

BENCH_SOURCE="$SCRIPT_DIR/pipelines.cpp"
BENCH_BINARY="$BIN_DIR/pipelines"
BENCH_OUTPUT="$RESULTS_DIR/pipelines-output.txt"

COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

cleanup() {
    sudo setcap -r "$BENCH_BINARY" 2>/dev/null || true
    rm -f "$BENCH_BINARY"
}

trap cleanup EXIT

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"
mkdir -p "$RESULTS_DIR"

rm -f "$BENCH_BINARY"
rm -f "$BENCH_OUTPUT"

g++-15 \
    @"$COMPILE_FLAGS_FILE" \
    "$BENCH_SOURCE" \
    -o "$BENCH_BINARY" \
    -O3 \
    -pthread

# Allow only performance-counter access for this temporary binary.
sudo setcap cap_perfmon=ep "$BENCH_BINARY"

echo "Running benchmark..."
echo "Output: $BENCH_OUTPUT"

"$BENCH_BINARY" "$@" 2>&1 | tee "$BENCH_OUTPUT"

echo "done!"