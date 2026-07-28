#!/usr/bin/env bash

set -Eeuo pipefail

readonly BUILD_SCRIPT_VERSION="2026.07.28-v6"

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BIN_ROOT="$ROOT_DIR/bin"
COMPILE_FLAGS_FILE="$ROOT_DIR/compile_flags.txt"

ACTION=""
CXX_REQUEST=""
CXX=""
CXX_KIND=""
CXX_MAJOR=""
PYTHON_REQUEST="python3"
PYTHON=""
GCOV_TOOL=""
LLVM_COV_TOOL=""
LLVM_PROFDATA_TOOL=""
TARGET_INPUT=""
TARGET_PATH=""
TARGET_KIND=""
TARGET_IS_FILE=false

SOURCES=()
FORWARDED_ARGS=()
STRICT_WARNING_FLAGS=()
COVERAGE_COMPILE_FLAGS=()

readonly -a COMMON_WARNING_FLAGS=(
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
)

readonly -a GCC_WARNING_FLAGS=(
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

# Clang does not support GCC's duplicated/logical-op/useless-cast warnings.
# These provide useful Clang-specific coverage without enabling -Weverything,
# which is not practical with third-party headers and -Werror.
readonly -a CLANG_WARNING_FLAGS=(
    -Wextra-semi
    -Wimplicit-fallthrough
    -Wnewline-eof
    -Wzero-as-null-pointer-constant
)

usage() {
    cat <<'USAGE'
XTD build 2026.07.28-v6

Usage:
  ./build.sh samples[/file.cpp] [+run] [options] [-- arguments]
  ./build.sh tests[/file.cpp] [+coverage|+coverage-webserver] [options] [-- arguments]
  ./build.sh benchmarks[/file.cpp] [options] [-- arguments]

Defaults:
  samples       Build only; +run builds and runs one file
  tests         Build, run, and write JUnit reports
  benchmarks    Build, run, and write benchmarks/results/*.md

Options:
  --cxx=<compiler>     Defaults to g++, then clang++
  --python=<python>    Web-server Python; defaults to python3

Examples:
  ./build.sh samples
  ./build.sh samples/channel.cpp +run bounded
  ./build.sh tests
  ./build.sh tests/pipelines.cpp +coverage --cxx=clang++-22
  ./build.sh tests +coverage-webserver --python=python3.13
  ./build.sh benchmarks --cxx=g++-15
USAGE
}

die() {
    echo "error: $*" >&2
    exit 1
}

require_command() {
    command -v -- "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

set_action() {
    local requested="$1"

    if [[ -n "$ACTION" && "$ACTION" != "$requested" ]]; then
        die "actions are mutually exclusive: --$ACTION and --$requested"
    fi

    ACTION="$requested"
}

parse_arguments() {
    if (($# == 0)); then
        usage >&2
        exit 1
    fi

    case "$1" in
        --version)
            echo "$BUILD_SCRIPT_VERSION"
            exit 0
            ;;
        -h|--help)
            usage
            exit 0
            ;;
    esac

    TARGET_INPUT="$1"
    shift

    while (($# > 0)); do
        case "$1" in
            +run)
                set_action run
                ;;
            +coverage)
                set_action coverage
                ;;
            +coverage-webserver)
                set_action coverage-webserver
                ;;
            --cxx=*)
                CXX_REQUEST="${1#--cxx=}"
                [[ -n "$CXX_REQUEST" ]] || die "--cxx requires a compiler executable"
                ;;
            --python=*)
                PYTHON_REQUEST="${1#--python=}"
                [[ -n "$PYTHON_REQUEST" ]] || die "--python requires a Python executable"
                ;;
            --version)
                echo "$BUILD_SCRIPT_VERSION"
                exit 0
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            --)
                shift
                FORWARDED_ARGS+=("$@")
                break
                ;;
            *)
                FORWARDED_ARGS+=("$1")
                ;;
        esac
        shift
    done
}

resolve_target() {
    local candidate

    if [[ "$TARGET_INPUT" == /* ]]; then
        candidate="$TARGET_INPUT"
    else
        candidate="$ROOT_DIR/$TARGET_INPUT"
    fi

    TARGET_PATH="$(realpath -e -- "$candidate" 2>/dev/null)" \
        || die "target not found: $TARGET_INPUT"

    case "$TARGET_PATH" in
        "$ROOT_DIR"/*) ;;
        *) die "target must be inside the repository: $TARGET_INPUT" ;;
    esac

    local relative_target="${TARGET_PATH#"$ROOT_DIR"/}"
    TARGET_KIND="${relative_target%%/*}"

    case "$TARGET_KIND" in
        samples|tests|benchmarks) ;;
        *) die "target must be inside samples, tests, or benchmarks: $TARGET_INPUT" ;;
    esac

    if [[ -f "$TARGET_PATH" ]]; then
        [[ "$TARGET_PATH" == *.cpp ]] || die "file target must have a .cpp extension: $TARGET_INPUT"
        TARGET_IS_FILE=true
        SOURCES=("$TARGET_PATH")
        return
    fi

    [[ -d "$TARGET_PATH" ]] || die "target must be a directory or .cpp file: $TARGET_INPUT"

    mapfile -d '' SOURCES < <(
        find "$TARGET_PATH" \
            -maxdepth 1 \
            -type f \
            -name '*.cpp' \
            -print0 \
            | sort -z
    )

    ((${#SOURCES[@]} > 0)) || die "no .cpp files found in target: $TARGET_INPUT"
}

configure_samples_target() {
    case "${ACTION:-build}" in
        build)
            ACTION=build

            ((${#FORWARDED_ARGS[@]} == 0)) \
                || die "samples build by default; use +run to pass arguments"
            ;;
        run)
            [[ "$TARGET_IS_FILE" == true ]] \
                || die "+run requires one samples/*.cpp target"
            ;;
        *)
            die "--$ACTION is not supported for samples"
            ;;
    esac
}

configure_tests_target() {
    case "${ACTION:-test}" in
        test)
            ACTION=test
            ;;
        coverage|coverage-webserver)
            ;;
        *)
            die "--$ACTION is not supported for tests"
            ;;
    esac
}

configure_benchmarks_target() {
    [[ -z "$ACTION" ]] \
        || die "benchmarks do not accept an action flag"

    ACTION=benchmark
}

configure_target() {
    case "$TARGET_KIND" in
        samples)
            configure_samples_target
            ;;
        tests)
            configure_tests_target
            ;;
        benchmarks)
            configure_benchmarks_target
            ;;
    esac
}

compiler_accepts_flags() {
    local probe_dir
    local status

    probe_dir="$(mktemp -d "${TMPDIR:-/tmp}/xtd-build-probe.XXXXXX")" \
        || return 1

    printf 'int main() { return 0; }\n' >"$probe_dir/probe.cpp"

    if (
        cd -- "$probe_dir"

        "$CXX" \
            -x c++ \
            -fsyntax-only \
            -Werror \
            "$@" \
            probe.cpp \
            >/dev/null 2>&1
    ); then
        status=0
    else
        status=$?
    fi

    rm -rf -- "$probe_dir"
    return "$status"
}

compiler_accepts_flag() {
    compiler_accepts_flags "$1"
}

append_supported_flags() {
    local output_name="$1"
    shift

    local -n output="$output_name"
    local flag

    for flag in "$@"; do
        if compiler_accepts_flag "$flag"; then
            output+=("$flag")
        else
            echo "Skipping unsupported compiler flag: $flag" >&2
        fi
    done
}

compiler_version_major() {
    local version=""

    version="$("$CXX" -dumpfullversion -dumpversion 2>/dev/null || true)"

    if [[ "$version" =~ ^([0-9]+) ]]; then
        printf '%s' "${BASH_REMATCH[1]}"
        return
    fi

    version="$("$CXX" --version 2>&1 | head -n 1)"

    if [[ "$version" =~ ([0-9]+)(\.[0-9]+)+ ]]; then
        printf '%s' "${BASH_REMATCH[1]}"
    fi
}

resolve_compiler() {
    local compiler_path

    if [[ -n "$CXX_REQUEST" ]]; then
        compiler_path="$(command -v -- "$CXX_REQUEST" 2>/dev/null)" \
            || die "C++ compiler not found: $CXX_REQUEST"
    elif command -v g++ >/dev/null 2>&1; then
        compiler_path="$(command -v g++)"
    elif command -v clang++ >/dev/null 2>&1; then
        compiler_path="$(command -v clang++)"
    else
        die "no C++ compiler found; install g++ or clang++, or pass --cxx=<compiler>"
    fi

    # Keep the selected driver path as invoked. Resolving clang++ to clang can
    # change link-driver behavior and omit the C++ standard library.
    CXX="$compiler_path"

    local version_text
    version_text="$("$CXX" --version 2>&1 | head -n 1)"
    CXX_MAJOR="$(compiler_version_major)"

    STRICT_WARNING_FLAGS=(-Werror)

    if [[ "$version_text" == *[Cc]lang* ]]; then
        CXX_KIND=clang

        append_supported_flags STRICT_WARNING_FLAGS \
            "${COMMON_WARNING_FLAGS[@]}" \
            "${CLANG_WARNING_FLAGS[@]}"

        if [[ "$ACTION" == coverage || "$ACTION" == coverage-webserver ]]; then
            compiler_accepts_flags \
                -fprofile-instr-generate \
                -fcoverage-mapping \
                || die "selected Clang does not support source-based coverage: $CXX"

            COVERAGE_COMPILE_FLAGS=(
                -O0
                -g
                -fprofile-instr-generate
                -fcoverage-mapping
            )

            if compiler_accepts_flags \
                -fprofile-instr-generate \
                -fcoverage-mapping \
                -fprofile-update=atomic
            then
                COVERAGE_COMPILE_FLAGS+=(-fprofile-update=atomic)
            fi
        fi
    elif [[ "$version_text" == *GCC* || "$version_text" == *g++* || "$version_text" == *gcc* ]]; then
        CXX_KIND=gcc

        append_supported_flags STRICT_WARNING_FLAGS \
            "${COMMON_WARNING_FLAGS[@]}" \
            "${GCC_WARNING_FLAGS[@]}"

        if [[ "$ACTION" == coverage || "$ACTION" == coverage-webserver ]]; then
            compiler_accepts_flag --coverage \
                || die "selected GCC does not support coverage instrumentation: $CXX"

            COVERAGE_COMPILE_FLAGS=(
                -O0
                -g
                --coverage
            )

            append_supported_flags COVERAGE_COMPILE_FLAGS \
                -fprofile-abs-path \
                -fprofile-update=atomic
        fi
    else
        die "unsupported compiler: $version_text"
    fi

    echo "Build script: $BUILD_SCRIPT_VERSION (${BASH_SOURCE[0]})"
    echo "Compiler: $version_text"
}

validate_repository() {
    [[ -f "$COMPILE_FLAGS_FILE" ]] \
        || die "compile flags file not found: $COMPILE_FLAGS_FILE"
}

relative_source() {
    local source="$1"
    printf '%s' "${source#"$ROOT_DIR"/}"
}

binary_for_source() {
    local source="$1"
    local relative
    relative="$(relative_source "$source")"
    printf '%s/%s' "$BIN_ROOT" "${relative%.cpp}"
}

relative_within_kind() {
    local source="$1"
    local relative
    relative="$(relative_source "$source")"
    printf '%s' "${relative#*/}"
}

compile_source() {
    local source="$1"
    local mode="$2"
    local binary
    binary="$(binary_for_source "$source")"

    mkdir -p -- "$(dirname -- "$binary")"
    rm -f -- "$binary"

    local -a flags=(
        -pthread
    )

    case "$mode" in
        sample)
            ;;
        benchmark)
            flags+=(
                -O3
            )
            ;;
        test)
            flags+=(
                "${STRICT_WARNING_FLAGS[@]}"
                -O3
            )
            ;;
        coverage)
            flags+=(
                "${STRICT_WARNING_FLAGS[@]}"
                "${COVERAGE_COMPILE_FLAGS[@]}"
            )
            ;;
        *)
            die "unknown compile mode: $mode"
            ;;
    esac

    echo "Building $(relative_source "$source") -> ${binary#"$ROOT_DIR"/}"

    "$CXX" \
        @"$COMPILE_FLAGS_FILE" \
        "$source" \
        -o "$binary" \
        "${flags[@]}"
}

build_samples() {
    local source
    for source in "${SOURCES[@]}"; do
        compile_source "$source" sample
    done

    echo "Built ${#SOURCES[@]} sample(s) into: $BIN_ROOT/samples"
}

run_sample() {
    local source="${SOURCES[0]}"
    local binary

    compile_source "$source" sample
    binary="$(binary_for_source "$source")"

    echo "Running ${binary#"$ROOT_DIR"/}..."
    "$binary" "${FORWARDED_ARGS[@]}"
}

run_benchmarks() {
    local source relative output binary

    mkdir -p -- "$ROOT_DIR/benchmarks/results"

    for source in "${SOURCES[@]}"; do
        compile_source "$source" benchmark

        relative="$(relative_within_kind "$source")"
        output="$ROOT_DIR/benchmarks/results/${relative%.cpp}.md"
        binary="$(binary_for_source "$source")"

        mkdir -p -- "$(dirname -- "$output")"
        rm -f -- "$output"

        echo "Running benchmark: $(relative_source "$source")"
        echo "Output: ${output#"$ROOT_DIR"/}"

        "$binary" "${FORWARDED_ARGS[@]}" 2>&1 | tee "$output"
    done
}

test_result_for_source() {
    local source="$1"
    local relative
    relative="$(relative_within_kind "$source")"
    printf '%s/tests/test-results/%s.xml' "$ROOT_DIR" "${relative%.cpp}"
}

prepare_test_results() {
    local results_dir="$ROOT_DIR/tests/test-results"

    if [[ "$TARGET_IS_FILE" == true ]]; then
        mkdir -p -- "$results_dir"
        rm -f -- "$(test_result_for_source "${SOURCES[0]}")"
    else
        rm -rf -- "$results_dir"
        mkdir -p -- "$results_dir"
    fi
}

run_tests() {
    prepare_test_results

    local source binary result
    local test_status=0

    for source in "${SOURCES[@]}"; do
        compile_source "$source" test
    done

    for source in "${SOURCES[@]}"; do
        binary="$(binary_for_source "$source")"
        result="$(test_result_for_source "$source")"
        mkdir -p -- "$(dirname -- "$result")"

        echo "Running test: $(relative_source "$source")"

        if "$binary" \
            "${FORWARDED_ARGS[@]}" \
            --reporters=junit \
            --out="$result"
        then
            echo "Passed: $(relative_source "$source")"
        else
            echo "Failed: $(relative_source "$source")" >&2
            test_status=1
        fi

        if [[ ! -s "$result" ]]; then
            echo "JUnit report was not generated: $result" >&2
            test_status=1
        else
            echo "JUnit report: ${result#"$ROOT_DIR"/}"
        fi
    done

    return "$test_status"
}

coverage_name_for_source() {
    local source="$1"
    local relative
    relative="$(relative_within_kind "$source")"
    printf '%s' "${relative%.cpp}"
}

coverage_trace_for_source() {
    local source="$1"
    local coverage_dir="$2"
    local name
    name="$(coverage_name_for_source "$source")"
    printf '%s/%s.raw.info' "$coverage_dir" "$name"
}

tool_major_version() {
    local tool="$1"
    local version_text

    version_text="$("$tool" --version 2>&1 | head -n 3)"

    if [[ "$version_text" =~ ([0-9]+)(\.[0-9]+)+ ]]; then
        printf '%s' "${BASH_REMATCH[1]}"
    fi
}

add_tool_candidate() {
    local output_name="$1"
    local candidate="$2"
    local -n output="$output_name"
    local resolved=""
    local existing

    [[ -n "$candidate" ]] || return 0

    if [[ "$candidate" == */* ]]; then
        [[ -x "$candidate" ]] || return 0
        resolved="$candidate"
    else
        resolved="$(command -v -- "$candidate" 2>/dev/null || true)"
        [[ -n "$resolved" ]] || return 0
    fi

    for existing in "${output[@]:-}"; do
        [[ "$existing" == "$resolved" ]] && return 0
    done

    output+=("$resolved")
}

compiler_program_path() {
    local program="$1"
    local result=""

    result="$("$CXX" -print-prog-name="$program" 2>/dev/null || true)"
    [[ "$result" != "$program" ]] || result=""
    printf '%s' "$result"
}

resolve_gcov_tool() {
    local printed compiler_dir real_compiler_dir candidate candidate_major
    local -a candidates=()

    printed="$(compiler_program_path gcov)"
    compiler_dir="$(dirname -- "$CXX")"
    real_compiler_dir="$(dirname -- "$(realpath -e -- "$CXX" 2>/dev/null || printf '%s' "$CXX")")"

    add_tool_candidate candidates "$printed"
    add_tool_candidate candidates "$compiler_dir/gcov"
    add_tool_candidate candidates "$real_compiler_dir/gcov"

    if [[ -n "$CXX_MAJOR" ]]; then
        add_tool_candidate candidates "gcov-$CXX_MAJOR"
    fi

    add_tool_candidate candidates gcov

    for candidate in "${candidates[@]}"; do
        candidate_major="$(tool_major_version "$candidate")"

        if [[ -z "$CXX_MAJOR" || -z "$candidate_major" || "$candidate_major" == "$CXX_MAJOR" ]]; then
            GCOV_TOOL="$candidate"
            return
        fi
    done

    die "no gcov compatible with $CXX was found"
}

resolve_llvm_tool() {
    local tool_name="$1"
    local printed compiler_dir real_compiler_dir candidate candidate_major
    local -a candidates=()

    printed="$(compiler_program_path "$tool_name")"
    compiler_dir="$(dirname -- "$CXX")"
    real_compiler_dir="$(dirname -- "$(realpath -e -- "$CXX" 2>/dev/null || printf '%s' "$CXX")")"

    add_tool_candidate candidates "$printed"
    add_tool_candidate candidates "$compiler_dir/$tool_name"
    add_tool_candidate candidates "$real_compiler_dir/$tool_name"

    if [[ -n "$CXX_MAJOR" ]]; then
        add_tool_candidate candidates "${tool_name}-${CXX_MAJOR}"
        add_tool_candidate candidates "/usr/lib/llvm-${CXX_MAJOR}/bin/${tool_name}"
    fi

    add_tool_candidate candidates "$tool_name"

    for candidate in "${candidates[@]}"; do
        candidate_major="$(tool_major_version "$candidate")"

        # LLVM raw profile formats are version-sensitive. Prefer/require a
        # matching major whenever the compiler exposes one.
        if [[ -z "$CXX_MAJOR" || -z "$candidate_major" || "$candidate_major" == "$CXX_MAJOR" ]]; then
            printf '%s' "$candidate"
            return
        fi
    done

    die "no $tool_name compatible with $CXX was found"
}

resolve_coverage_tools() {
    case "$CXX_KIND" in
        gcc)
            resolve_gcov_tool
            echo "Coverage backend: GCC gcov/LCOV"
            echo "GCC coverage tool: $GCOV_TOOL"
            ;;
        clang)
            LLVM_COV_TOOL="$(resolve_llvm_tool llvm-cov)"
            LLVM_PROFDATA_TOOL="$(resolve_llvm_tool llvm-profdata)"

            echo "Coverage backend: Clang source-based coverage"
            echo "LLVM coverage tool: $LLVM_COV_TOOL"
            echo "LLVM profile tool: $LLVM_PROFDATA_TOOL"
            ;;
    esac
}

capture_gcc_coverage() {
    local source="$1"
    local suite_coverage="$2"
    local test_bin_dir="$3"

    lcov \
        --capture \
        --directory "$test_bin_dir" \
        --base-directory "$ROOT_DIR" \
        --no-external \
        --branch-coverage \
        --test-name "$(coverage_name_for_source "$source")" \
        --gcov-tool "$GCOV_TOOL" \
        --rc geninfo_gcov_all_blocks=0 \
        --rc geninfo_unexecuted_blocks=0 \
        --ignore-errors inconsistent,inconsistent,mismatch,mismatch \
        --output-file "$suite_coverage"
}

capture_clang_coverage() {
    local source="$1"
    local binary="$2"
    local suite_coverage="$3"
    local profile_dir="$4"
    local name profdata
    local -a raw_profiles=()

    name="$(coverage_name_for_source "$source")"
    profdata="${suite_coverage%.raw.info}.profdata"

    mapfile -d '' raw_profiles < <(
        find "$profile_dir" \
            -maxdepth 1 \
            -type f \
            -name '*.profraw' \
            -print0 \
            | sort -z
    )

    if ((${#raw_profiles[@]} == 0)); then
        echo "No LLVM raw profiles were generated for: $(relative_source "$source")" >&2
        return 1
    fi

    if ! "$LLVM_PROFDATA_TOOL" merge \
        -sparse \
        "${raw_profiles[@]}" \
        -o "$profdata"
    then
        return 1
    fi

    # llvm-cov's native LCOV exporter avoids the malformed synthetic
    # __cxx_global_var_init line-0 records produced by llvm-cov gcov.
    {
        printf 'TN:%s\n' "$name"
        "$LLVM_COV_TOOL" export \
            -format=lcov \
            -instr-profile="$profdata" \
            "$binary"
    } >"$suite_coverage"

    [[ -s "$suite_coverage" ]]
}

write_coverage_descriptions() {
    local output="$1"
    local source name description

    : >"$output"

    for source in "${SOURCES[@]}"; do
        name="$(coverage_name_for_source "$source")"

        case "$name" in
            channels)
                description="Channel tests"
                ;;
            pipelines)
                description="Pipeline tests"
                ;;
            *)
                description="$name tests"
                ;;
        esac

        printf 'TN:%s\nTD:%s\n\n' "$name" "$description" >>"$output"
    done
}

run_coverage() {
    require_command lcov
    require_command genhtml

    local coverage_dir="$ROOT_DIR/tests/coverage"
    local raw_coverage="$coverage_dir/coverage.raw.info"
    local project_coverage="$coverage_dir/coverage.project.info"
    local filtered_coverage="$coverage_dir/coverage.info"
    local html_coverage="$coverage_dir/html"
    local descriptions="$coverage_dir/test-descriptions.txt"
    local test_results_dir="$ROOT_DIR/tests/test-results"
    local test_bin_dir="$BIN_ROOT/tests"

    rm -rf -- "$coverage_dir" "$test_results_dir"
    mkdir -p -- "$coverage_dir" "$html_coverage" "$test_results_dir" "$test_bin_dir"

    write_coverage_descriptions "$descriptions"

    find "$test_bin_dir" \
        -type f \
        \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' \) \
        -delete

    resolve_coverage_tools

    local source binary result suite_coverage
    local test_status=0
    local coverage_status=0
    local -a suite_coverage_files=()

    echo "Building tests with coverage..."
    for source in "${SOURCES[@]}"; do
        compile_source "$source" coverage
    done

    echo "Running tests and capturing per-suite coverage..."

    for source in "${SOURCES[@]}"; do
        binary="$(binary_for_source "$source")"
        result="$(test_result_for_source "$source")"
        suite_coverage="$(coverage_trace_for_source "$source" "$coverage_dir")"

        mkdir -p -- "$(dirname -- "$result")" "$(dirname -- "$suite_coverage")"

        local profile_dir=""
        local profile_pattern=""

        case "$CXX_KIND" in
            gcc)
                find "$test_bin_dir" -type f -name '*.gcda' -delete
                ;;
            clang)
                profile_dir="$coverage_dir/profiles/$(coverage_name_for_source "$source")"
                rm -rf -- "$profile_dir"
                mkdir -p -- "$profile_dir"
                profile_pattern="$profile_dir/%p.profraw"
                ;;
        esac

        echo "Running test: $(relative_source "$source")"

        if [[ "$CXX_KIND" == clang ]]; then
            if LLVM_PROFILE_FILE="$profile_pattern" \
                "$binary" \
                "${FORWARDED_ARGS[@]}" \
                --reporters=junit \
                --out="$result"
            then
                echo "Passed: $(relative_source "$source")"
            else
                echo "Failed: $(relative_source "$source")" >&2
                test_status=1
            fi
        elif "$binary" \
            "${FORWARDED_ARGS[@]}" \
            --reporters=junit \
            --out="$result"
        then
            echo "Passed: $(relative_source "$source")"
        else
            echo "Failed: $(relative_source "$source")" >&2
            test_status=1
        fi

        if [[ ! -s "$result" ]]; then
            echo "JUnit report was not generated: $result" >&2
            test_status=1
        else
            echo "JUnit report: ${result#"$ROOT_DIR"/}"
        fi

        echo "Capturing coverage: $(coverage_name_for_source "$source")"

        local capture_status=0

        case "$CXX_KIND" in
            gcc)
                capture_gcc_coverage \
                    "$source" \
                    "$suite_coverage" \
                    "$test_bin_dir" \
                    || capture_status=$?
                ;;
            clang)
                capture_clang_coverage \
                    "$source" \
                    "$binary" \
                    "$suite_coverage" \
                    "$profile_dir" \
                    || capture_status=$?
                ;;
        esac

        if ((capture_status == 0)); then
            suite_coverage_files+=("$suite_coverage")
        else
            echo "Coverage capture failed: $(relative_source "$source")" >&2
            coverage_status=1
        fi
    done

    ((${#suite_coverage_files[@]} > 0)) \
        || die "no coverage tracefiles were generated"

    local -a lcov_add_arguments=()
    for suite_coverage in "${suite_coverage_files[@]}"; do
        lcov_add_arguments+=(--add-tracefile "$suite_coverage")
    done

    echo "Combining coverage tracefiles..."
    lcov \
        "${lcov_add_arguments[@]}" \
        --branch-coverage \
        --ignore-errors inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch \
        --output-file "$raw_coverage"

    echo "Keeping repository coverage only..."
    lcov \
        --extract "$raw_coverage" \
        "$ROOT_DIR/*" \
        --branch-coverage \
        --ignore-errors unused,unused,inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch \
        --output-file "$project_coverage"

    echo "Excluding third-party code and test files..."
    lcov \
        --remove "$project_coverage" \
        '*/third_party/*' \
        '*/tests/*' \
        --branch-coverage \
        --filter brace,branch,exception \
        --ignore-errors unused,unused,inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch \
        --output-file "$filtered_coverage"

    echo "Generating combined HTML report..."
    genhtml \
        "$filtered_coverage" \
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
        --description-file "$descriptions" \
        --output-directory "$html_coverage"

    echo
    echo "Combined coverage summary:"
    lcov \
        --summary "$filtered_coverage" \
        --branch-coverage

    echo
    echo "Coverage report: ${html_coverage#"$ROOT_DIR"/}/index.html"
    echo "JUnit reports: ${test_results_dir#"$ROOT_DIR"/}"

    if ((test_status != 0 || coverage_status != 0)); then
        die "one or more tests or coverage captures failed"
    fi

    if [[ "$ACTION" == coverage-webserver ]]; then
        PYTHON="$(command -v -- "$PYTHON_REQUEST" 2>/dev/null)" \
            || die "Python executable not found: $PYTHON_REQUEST"

        echo
        echo "Python: $($PYTHON --version 2>&1)"
        echo "Coverage report available at: http://localhost:8000"
        exec "$PYTHON" -m http.server 8000 --directory "$html_coverage"
    fi
}

execute_samples_target() {
    case "$ACTION" in
        build)
            build_samples
            ;;
        run)
            run_sample
            ;;
    esac
}

execute_tests_target() {
    case "$ACTION" in
        test)
            run_tests
            ;;
        coverage|coverage-webserver)
            run_coverage
            ;;
    esac
}

execute_benchmarks_target() {
    run_benchmarks
}

execute_target() {
    case "$TARGET_KIND" in
        samples)
            execute_samples_target
            ;;
        tests)
            execute_tests_target
            ;;
        benchmarks)
            execute_benchmarks_target
            ;;
    esac
}

main() {
    parse_arguments "$@"
    validate_repository
    resolve_target
    configure_target
    resolve_compiler

    mkdir -p -- "$BIN_ROOT"
    cd -- "$ROOT_DIR"

    execute_target
}
main "$@"