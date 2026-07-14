#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
SAMPLES_DIR="$SCRIPT_DIR"
BIN_DIR="$SAMPLES_DIR/bin"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

if [[ ! -d "$SAMPLES_DIR" ]]; then
    echo "Samples directory not found: $SAMPLES_DIR" >&2
    exit 1
fi

if [[ ! -f "$COMPILE_FLAGS_FILE" ]]; then
    echo "Compile flags file not found: $COMPILE_FLAGS_FILE" >&2
    exit 1
fi

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"

shopt -s nullglob
sample_files=("$SAMPLES_DIR"/*.cpp)
shopt -u nullglob

if [[ ${#sample_files[@]} -eq 0 ]]; then
    echo "No .cpp files found in: $SAMPLES_DIR"
    exit 0
fi

for source_file in "${sample_files[@]}"; do
    sample_name="$(basename -- "$source_file" .cpp)"
    binary_file="$BIN_DIR/$sample_name"

    echo "Building $sample_name..."
    g++-15 @"$COMPILE_FLAGS_FILE" "$source_file" -o "$binary_file"
done

echo "Built ${#sample_files[@]} sample(s) into: $BIN_DIR"
