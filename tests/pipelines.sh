#!/bin/bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
BIN_DIR="$SCRIPT_DIR/bin"
TEST_SOURCE="$SCRIPT_DIR/pipelines.cpp"
TEST_BINARY="$BIN_DIR/tests"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"

rm -f "$TEST_BINARY"
echo "Building tests..."
g++-15 "$TEST_SOURCE" -o "$TEST_BINARY" -O3 -pthread @"$COMPILE_FLAGS_FILE" 
echo "Running tests..."
"$TEST_BINARY" #--duration=true #--success=true "$@"

# rm -f "$TEST_BINARY"
# echo "Building tests..."
# clang++ "$TEST_SOURCE" -o "$TEST_BINARY" -O3 -pthread @"$COMPILE_FLAGS_FILE"
# echo "Running tests..."
# "$TEST_BINARY" #--duration=true #--success=true "$@"

rm -f "$TEST_BINARY"
