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
COVERAGE_DESCRIPTIONS="$COVERAGE_DIR/test-descriptions.txt"

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

COVERAGE_COMPILE_FLAGS=(
    -O0
    -g
    --coverage
    -fprofile-abs-path
    -fprofile-update=atomic
)

TEST_BINARIES=()
SUITE_COVERAGE_FILES=()

TEST_STATUS=0
COVERAGE_STATUS=0

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

# Remove previously generated reports.
rm -rf "$COVERAGE_DIR"
rm -rf "$TEST_RESULTS_DIR"

mkdir -p "$COVERAGE_DIR"
mkdir -p "$TEST_RESULTS_DIR"

cat >"$COVERAGE_DESCRIPTIONS" <<'EOF'
TN:channels
TD:Channel tests

TN:pipelines
TD:Pipeline, segmented byte view, fixed buffer pool, and position tests
EOF

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

    g++ \
        "$test_source" \
        -o "$test_binary" \
        "${TEST_WARNING_FLAGS[@]}" \
        "${COVERAGE_COMPILE_FLAGS[@]}" \
        -pthread \
        @"$COMPILE_FLAGS_FILE"
done

echo
echo "Running tests and capturing per-suite coverage..."

for index in "${!TEST_BINARIES[@]}"; do
    test_name="${TEST_NAMES[$index]}"
    test_binary="${TEST_BINARIES[$index]}"
    test_result="$TEST_RESULTS_DIR/${test_name}.xml"
    suite_coverage="$COVERAGE_DIR/${test_name}.raw.info"

    # Keep coverage from each suite independent.
    find "$BIN_DIR" -type f -name '*.gcda' -delete

    echo
    echo "  Running $(basename "$test_binary")..."

    if "$test_binary" \
        "$@" \
        --reporters=junit \
        --out="$test_result"
    then
        echo "  Passed: $test_name"
    else
        echo "  Failed: $test_name" >&2
        TEST_STATUS=1
    fi

    if [[ ! -s "$test_result" ]]; then
        echo "JUnit report was not generated: $test_result" >&2
        TEST_STATUS=1
    else
        echo "  JUnit report: $test_result"
    fi

    echo "  Capturing coverage for $test_name..."

    if lcov \
        --capture \
        --directory "$BIN_DIR" \
        --base-directory "$ROOT_DIR" \
        --no-external \
        --branch-coverage \
        --test-name "$test_name" \
        --gcov-tool gcov \
        --rc geninfo_gcov_all_blocks=0 \
        --rc geninfo_unexecuted_blocks=0 \
        --ignore-errors inconsistent,inconsistent,mismatch,mismatch \
        --output-file "$suite_coverage"
    then
        SUITE_COVERAGE_FILES+=("$suite_coverage")
    else
        echo "Coverage capture failed for: $test_name" >&2
        COVERAGE_STATUS=1
    fi
done

if ((${#SUITE_COVERAGE_FILES[@]} == 0)); then
    echo "No coverage tracefiles were generated." >&2
    exit 1
fi

echo
echo "Combining coverage tracefiles..."

LCOV_ADD_ARGUMENTS=()

for suite_coverage in "${SUITE_COVERAGE_FILES[@]}"; do
    LCOV_ADD_ARGUMENTS+=(
        --add-tracefile "$suite_coverage"
    )
done

lcov \
    "${LCOV_ADD_ARGUMENTS[@]}" \
    --branch-coverage \
    --ignore-errors \
        inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch \
    --output-file "$RAW_COVERAGE"

echo "Excluding third-party code and test files..."

lcov \
    --remove "$RAW_COVERAGE" \
    '*/third_party/*' \
    '*/tests/*' \
    --branch-coverage \
    --filter brace,branch,exception \
    --ignore-errors \
        unused,unused,inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch \
    --output-file "$FILTERED_COVERAGE"

echo "Generating combined HTML report..."

genhtml \
    "$FILTERED_COVERAGE" \
    --function-coverage \
    --branch-coverage \
    --demangle-cpp \
    --filter brace,branch,exception \
    --show-navigation \
    --show-proportion \
    --legend \
    --sort \
    --dark-mode \
    --precision 2 \
    --title "XTD Test Coverage" \
    --header-title "XTD Code Coverage" \
    --description-file "$COVERAGE_DESCRIPTIONS" \
    --output-directory "$HTML_COVERAGE"

echo
echo "Combined coverage summary:"

lcov \
    --summary "$FILTERED_COVERAGE" \
    --branch-coverage

echo
echo "Coverage report generated at:"
echo "$HTML_COVERAGE/index.html"

echo
echo "JUnit reports generated at:"
echo "$TEST_RESULTS_DIR"

if ((TEST_STATUS != 0 || COVERAGE_STATUS != 0)); then
    echo
    echo "One or more test suites or coverage captures failed." >&2
    exit 1
fi

if [[ "${CI:-false}" != "true" ]]; then
    echo
    echo "Coverage report available at:"
    echo "http://localhost:8000"

    python3 -m http.server 8000 --directory "$HTML_COVERAGE"
fi