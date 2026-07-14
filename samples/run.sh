#!/bin/bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
BIN_DIR="$SCRIPT_DIR/bin"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

cd "$ROOT_DIR"

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <sample.cpp> [args...]" >&2
    exit 1
fi

INPUT_NAME="$1"
shift

if [[ "$INPUT_NAME" != *.cpp ]]; then
    echo "Expected a .cpp sample file, got: $INPUT_NAME" >&2
    exit 1
fi

SAMPLE_NAME="$(basename -- "$INPUT_NAME" .cpp)"

SOURCE_FILE="$SCRIPT_DIR/$SAMPLE_NAME.cpp"
BINARY_FILE="$BIN_DIR/$SAMPLE_NAME"

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Sample source not found: $SOURCE_FILE" >&2
    exit 1
fi

mkdir -p "$BIN_DIR"
g++-15 @"$COMPILE_FLAGS_FILE" "$SOURCE_FILE" -o "$BINARY_FILE"
"$BINARY_FILE" "$@"