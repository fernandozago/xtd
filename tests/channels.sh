#!/bin/bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
BIN_DIR="$SCRIPT_DIR/bin"
TEST_SOURCE="$SCRIPT_DIR/channels.cpp"
TEST_BINARY="$BIN_DIR/tests"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"

rm -f "$TEST_BINARY"
echo "Building Channels tests..."
g++-15 @"$COMPILE_FLAGS_FILE" "$TEST_SOURCE" -o "$TEST_BINARY" -O3
echo "Running Channels tests..."
"$TEST_BINARY" #--duration=true --success=true "$@"
echo "done!"

# rm -f "$TEST_BINARY"
# clang++ -std=c++20 -I"$INCLUDE_DIR" "$TEST_SOURCE" -o "$TEST_BINARY" -Wall -Wextra -O3
# echo "Running tests..."
# "$TEST_BINARY" #--duration=true --success=true "$@"
# echo "done!"

rm -f "$TEST_BINARY"
