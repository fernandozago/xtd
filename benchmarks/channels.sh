#!/bin/bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
BIN_DIR="$SCRIPT_DIR/bin"
BENCH_SOURCE="$SCRIPT_DIR/channels.cpp"
BENCH_BINARY="$BIN_DIR/channels"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"

rm -f "$BENCH_BINARY"
g++-15 @"$COMPILE_FLAGS_FILE" "$BENCH_SOURCE" -o "$BENCH_BINARY" -O3 -pthread

echo "Running benchmark..."
"$BENCH_BINARY" "$@"
echo "done!"

rm -f "$BENCH_BINARY"
