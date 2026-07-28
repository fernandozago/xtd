#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"

BIN_DIR="$SCRIPT_DIR/bin"
TEST_RESULTS_DIR="$SCRIPT_DIR/test-results"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

TEST_NAMES=(
    "channels"
    "pipelines"
)

COVERAGE_DIR="$SCRIPT_DIR/coverage"
RAW_COVERAGE="$COVERAGE_DIR/coverage.raw.info"
FILTERED_COVERAGE="$COVERAGE_DIR/coverage.info"
HTML_COVERAGE="$COVERAGE_DIR/html"

TEST_WARNING_FLAGS=(
    -Wpedantic
    -pedantic-errors
    -Werror
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wformat=2
    -Wundef
    -Wcast-align
    -Wcast-qual
    -Wold-style-cast
    -Woverloaded-virtual
    -Wnon-virtual-dtor
    -Wnull-dereference
    -Wdouble-promotion
    -Wswitch-enum
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

TEST_BINARIES=()

cleanup() {
    if ((${#TEST_BINARIES[@]} > 0)); then
        rm -f "${TEST_BINARIES[@]}"
    fi
}

trap cleanup EXIT

cd "$ROOT_DIR"

mkdir -p "$BIN_DIR"

if [[ ! -f "$COMPILE_FLAGS_FILE" ]]; then
    echo "Compile flags file not found: $COMPILE_FLAGS_FILE" >&2
    exit 1
fi

# Remove previous generated reports.
rm -rf "$COVERAGE_DIR"
rm -rf "$TEST_RESULTS_DIR"

# Remove stale GCC coverage data.
find "$BIN_DIR" -type f \
    \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' \) \
    -delete

echo "Building tests with coverage..."

for test_name in "${TEST_NAMES[@]}"; do
    test_source="$SCRIPT_DIR/${test_name}.cpp"
    test_binary="$BIN_DIR/${test_name}_tests"

    if [[ ! -f "$test_source" ]]; then
        echo "Test source not found: $test_source" >&2
        exit 1
    fi

    rm -f "$test_binary"
    TEST_BINARIES+=("$test_binary")

    echo "  Building ${test_name}.cpp..."

    g++-15 \
        "$test_source" \
        -o "$test_binary" \
        "${TEST_WARNING_FLAGS[@]}" \
        -O0 \
        -g \
        --coverage \
        -pthread \
        @"$COMPILE_FLAGS_FILE"
done

echo
echo "Running tests..."

if [[ "${CI:-false}" == "true" ]]; then
    mkdir -p "$TEST_RESULTS_DIR"
fi

for index in "${!TEST_BINARIES[@]}"; do
    test_name="${TEST_NAMES[$index]}"
    test_binary="${TEST_BINARIES[$index]}"

    echo "  Running $(basename "$test_binary")..."

    if [[ "${CI:-false}" == "true" ]]; then
        "$test_binary" \
            --reporters=junit \
            --out="$TEST_RESULTS_DIR/${test_name}.xml" \
            "$@"
    else
        "$test_binary" "$@"
    fi
done

mkdir -p "$COVERAGE_DIR"

echo
echo "Capturing combined coverage..."

lcov \
    --capture \
    --directory "$BIN_DIR" \
    --base-directory "$ROOT_DIR" \
    --no-external \
    --gcov-tool gcov-15 \
    --rc geninfo_gcov_all_blocks=0 \
    --rc geninfo_unexecuted_blocks=0 \
    --ignore-errors inconsistent \
    --output-file "$RAW_COVERAGE"

echo "Excluding doctest, third-party code, and test files..."

lcov \
    --remove "$RAW_COVERAGE" \
    '*/third_party/*' \
    '*/doctest.h' \
    '*/tests/*' \
    --ignore-errors unused \
    --output-file "$FILTERED_COVERAGE"

echo "Generating combined HTML report..."

genhtml \
    "$FILTERED_COVERAGE" \
    --demangle-cpp \
    --filter brace \
    --dark-mode \
    --output-directory "$HTML_COVERAGE"

echo
echo "Coverage summary:"
lcov --summary "$FILTERED_COVERAGE"

echo
echo "Coverage report generated at:"
echo "$HTML_COVERAGE/index.html"

if [[ "${CI:-false}" != "true" ]]; then
    echo
    echo "Coverage report available at:"
    echo "http://localhost:8000"

    python3 -m http.server 8000 --directory "$HTML_COVERAGE"
fi