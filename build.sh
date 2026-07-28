#!/usr/bin/env bash
set -Eeuo pipefail

readonly VERSION='2026.07.28-compact-v1'
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
BIN="$ROOT/bin"
FLAGS="$ROOT/compile_flags.txt"

ACTION= CXX_REQ= CXX= CXX_KIND= CXX_MAJOR= TARGET= KIND=
PYTHON_REQ=python3 GCOV= LLVM_COV= LLVM_PROFDATA=
IS_FILE=false
SOURCES=() ARGS=() WARN=() COV_FLAGS=()

readonly -a COMMON_WARN=(
  -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion -Wshadow
  -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wold-style-cast
  -Woverloaded-virtual -Wnon-virtual-dtor -Wnull-dereference
  -Wdouble-promotion -Wswitch-enum
)
readonly -a GCC_WARN=(-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast)
readonly -a CLANG_WARN=(-Wextra-semi -Wimplicit-fallthrough -Wnewline-eof -Wzero-as-null-pointer-constant)

usage() { cat <<'TXT'
XTD compact build script

Usage:
  ./build.sh samples[/file.cpp] [+run] [options] [-- arguments]
  ./build.sh tests[/file.cpp] [+coverage|+coverage-webserver] [options] [-- arguments]
  ./build.sh benchmarks[/file.cpp] [options] [-- arguments]

Options:
  --cxx=<compiler>     Defaults to g++, then clang++
  --python=<python>    Defaults to python3
  -h, --help
  --version
TXT
}

die() { printf 'error: %s\n' "$*" >&2; exit 1; }
have() { command -v -- "$1" >/dev/null 2>&1; }
need() { have "$1" || die "required command not found: $1"; }
rel() { printf '%s' "${1#"$ROOT"/}"; }
within_kind() { local x; x=$(rel "$1"); printf '%s' "${x#*/}"; }
bin_for() { local x; x=$(rel "$1"); printf '%s/%s' "$BIN" "${x%.cpp}"; }
test_xml() { local x; x=$(within_kind "$1"); printf '%s/tests/test-results/%s.xml' "$ROOT" "${x%.cpp}"; }
cov_name() { local x; x=$(within_kind "$1"); printf '%s' "${x%.cpp}"; }

set_action() {
  [[ -z "$ACTION" || "$ACTION" == "$1" ]] || die "actions are mutually exclusive"
  ACTION=$1
}

parse() {
  (($#)) || { usage >&2; exit 1; }
  case $1 in -h|--help) usage; exit;; --version) echo "$VERSION"; exit;; esac
  TARGET=$1; shift
  while (($#)); do
    case $1 in
      +run) set_action run ;;
      +coverage) set_action coverage ;;
      +coverage-webserver) set_action coverage-webserver ;;
      --cxx=*) CXX_REQ=${1#*=}; [[ $CXX_REQ ]] || die '--cxx requires a value' ;;
      --python=*) PYTHON_REQ=${1#*=}; [[ $PYTHON_REQ ]] || die '--python requires a value' ;;
      -h|--help) usage; exit ;;
      --version) echo "$VERSION"; exit ;;
      --) shift; ARGS+=("$@"); break ;;
      *) ARGS+=("$1") ;;
    esac
    shift
  done
}

resolve_target() {
  local p=$TARGET r
  [[ $p == /* ]] || p="$ROOT/$p"
  TARGET=$(realpath -e -- "$p" 2>/dev/null) || die "target not found: $p"
  case $TARGET in "$ROOT"/*) ;; *) die 'target must be inside the repository' ;; esac

  r=$(rel "$TARGET"); KIND=${r%%/*}
  [[ $KIND == samples || $KIND == tests || $KIND == benchmarks ]] ||
    die 'target must be inside samples, tests, or benchmarks'

  if [[ -f $TARGET ]]; then
    [[ $TARGET == *.cpp ]] || die 'file target must end in .cpp'
    IS_FILE=true; SOURCES=("$TARGET")
  else
    [[ -d $TARGET ]] || die 'target must be a directory or .cpp file'
    mapfile -d '' SOURCES < <(find "$TARGET" -maxdepth 1 -type f -name '*.cpp' -print0 | sort -z)
    ((${#SOURCES[@]})) || die 'no .cpp files found'
  fi

  case $KIND:${ACTION:-} in
    samples:) ACTION=build; ((${#ARGS[@]} == 0)) || die 'use +run to pass sample arguments' ;;
    samples:run) $IS_FILE || die '+run requires one samples/*.cpp file' ;;
    samples:*) die 'samples only support +run' ;;
    tests:) ACTION=test ;;
    tests:test|tests:coverage|tests:coverage-webserver) ;;
    tests:*) die 'tests only support coverage actions' ;;
    benchmarks:) ACTION=benchmark ;;
    benchmarks:*) die 'benchmarks do not accept an action' ;;
  esac
}

accepts() {
  local d s
  d=$(mktemp -d "${TMPDIR:-/tmp}/xtd-probe.XXXXXX") || return 1
  printf 'int main(){}\n' >"$d/p.cpp"
  (cd "$d" && "$CXX" -x c++ -fsyntax-only -Werror "$@" p.cpp >/dev/null 2>&1); s=$?
  rm -rf -- "$d"; return "$s"
}

add_supported() {
  local -n out=$1; shift
  local f
  for f; do accepts "$f" && out+=("$f") || printf 'Skipping unsupported flag: %s\n' "$f" >&2; done
}

major() {
  local v
  local tool=$1
  v=$("$tool" -dumpfullversion -dumpversion 2>/dev/null || "$tool" --version 2>&1 | head -1)
  [[ $v =~ ([0-9]+)(\.[0-9]+)* ]] && printf '%s' "${BASH_REMATCH[1]}"
}

resolve_compiler() {
  if [[ $CXX_REQ ]]; then CXX=$(command -v -- "$CXX_REQ" 2>/dev/null) || die "compiler not found: $CXX_REQ"
  elif have g++; then CXX=$(command -v g++)
  elif have clang++; then CXX=$(command -v clang++)
  else die 'install g++/clang++ or pass --cxx=<compiler>'; fi

  local v; v=$("$CXX" --version 2>&1 | head -1); CXX_MAJOR=$(major "$CXX" || true)
  WARN=(-Werror)
  if [[ $v == *[Cc]lang* ]]; then
    CXX_KIND=clang; add_supported WARN "${COMMON_WARN[@]}" "${CLANG_WARN[@]}"
    if [[ $ACTION == coverage* ]]; then
      accepts -fprofile-instr-generate -fcoverage-mapping || die 'Clang lacks source coverage support'
      COV_FLAGS=(-O0 -g -fprofile-instr-generate -fcoverage-mapping)
      accepts "${COV_FLAGS[@]}" -fprofile-update=atomic && COV_FLAGS+=(-fprofile-update=atomic)
    fi
  elif [[ $v == *GCC* || $v == *g++* || $v == *gcc* ]]; then
    CXX_KIND=gcc; add_supported WARN "${COMMON_WARN[@]}" "${GCC_WARN[@]}"
    if [[ $ACTION == coverage* ]]; then
      accepts --coverage || die 'GCC lacks coverage support'
      COV_FLAGS=(-O0 -g --coverage)
      add_supported COV_FLAGS -fprofile-abs-path -fprofile-update=atomic
    fi
  else die "unsupported compiler: $v"; fi
  printf 'Build script: %s\nCompiler: %s\n' "$VERSION" "$v"
}

compile() {
  local src=$1 mode=$2 out; out=$(bin_for "$src")
  mkdir -p "$(dirname "$out")"; rm -f "$out"
  local -a f=(-pthread)
  case $mode in
    benchmark) f+=(-O3) ;;
    test) f+=("${WARN[@]}" -O3) ;;
    coverage) f+=("${WARN[@]}" "${COV_FLAGS[@]}") ;;
  esac
  printf 'Building %s -> %s\n' "$(rel "$src")" "$(rel "$out")"
  "$CXX" @"$FLAGS" "$src" -o "$out" "${f[@]}"
}

build_all() { local s; for s in "${SOURCES[@]}"; do compile "$s" "$1"; done; }

run_tests() {
  local dir="$ROOT/tests/test-results" s b xml status=0
  if $IS_FILE; then mkdir -p "$dir"; rm -f "$(test_xml "${SOURCES[0]}")"; else rm -rf "$dir"; mkdir -p "$dir"; fi
  build_all test
  for s in "${SOURCES[@]}"; do
    b=$(bin_for "$s"); xml=$(test_xml "$s"); mkdir -p "$(dirname "$xml")"
    printf 'Running test: %s\n' "$(rel "$s")"
    "$b" "${ARGS[@]}" --reporters=junit --out="$xml" || status=1
    [[ -s $xml ]] || { printf 'JUnit report missing: %s\n' "$xml" >&2; status=1; }
  done
  return "$status"
}

run_benchmarks() {
  local s b out name
  for s in "${SOURCES[@]}"; do
    compile "$s" benchmark; b=$(bin_for "$s"); name=$(within_kind "$s")
    out="$ROOT/benchmarks/results/${name%.cpp}.md"; mkdir -p "$(dirname "$out")"
    printf 'Running benchmark: %s\nOutput: %s\n' "$(rel "$s")" "$(rel "$out")"
    "$b" "${ARGS[@]}" 2>&1 | tee "$out"
  done
}

add_candidate() {
  local -n a=$1; local x=$2 r e
  [[ $x ]] || return 0
  if [[ $x == */* ]]; then [[ -x $x ]] || return 0; r=$x
  else r=$(command -v -- "$x" 2>/dev/null || true); [[ $r ]] || return 0; fi
  for e in "${a[@]:-}"; do [[ $e == "$r" ]] && return; done
  a+=("$r")
}

tool_major() { local tool=$1 v; v=$("$tool" --version 2>&1 | head -3); [[ $v =~ ([0-9]+)(\.[0-9]+)+ ]] && echo "${BASH_REMATCH[1]}"; }

find_companion() {
  local name=$1 c m printed dir realdir; local -a list=()
  printed=$("$CXX" -print-prog-name="$name" 2>/dev/null || true); [[ $printed == "$name" ]] && printed=
  dir=$(dirname "$CXX"); realdir=$(dirname "$(realpath -e "$CXX" 2>/dev/null || echo "$CXX")")
  add_candidate list "$printed"; add_candidate list "$dir/$name"; add_candidate list "$realdir/$name"
  [[ $CXX_MAJOR ]] && { add_candidate list "$name-$CXX_MAJOR"; add_candidate list "/usr/lib/llvm-$CXX_MAJOR/bin/$name"; }
  add_candidate list "$name"
  for c in "${list[@]}"; do
    m=$(tool_major "$c" || true)
    [[ -z $CXX_MAJOR || -z $m || $m == "$CXX_MAJOR" ]] && { echo "$c"; return; }
  done
  die "no compatible $name found"
}

resolve_cov_tools() {
  if [[ $CXX_KIND == gcc ]]; then GCOV=$(find_companion gcov); echo "Coverage: GCC ($GCOV)"
  else LLVM_COV=$(find_companion llvm-cov); LLVM_PROFDATA=$(find_companion llvm-profdata); echo "Coverage: Clang ($LLVM_COV, $LLVM_PROFDATA)"; fi
}

capture_gcc() {
  local src=$1 out=$2
  lcov --capture --directory "$BIN/tests" --base-directory "$ROOT" --no-external --branch-coverage \
    --test-name "$(cov_name "$src")" --gcov-tool "$GCOV" \
    --rc geninfo_gcov_all_blocks=0 --rc geninfo_unexecuted_blocks=0 \
    --ignore-errors inconsistent,inconsistent,mismatch,mismatch --output-file "$out"
}

capture_clang() {
  local src=$1 bin=$2 out=$3 profiles=$4 data
  local -a raw=()
  data="${out%.raw.info}.profdata"
  mapfile -d '' raw < <(find "$profiles" -maxdepth 1 -type f -name '*.profraw' -print0 | sort -z)
  ((${#raw[@]})) || return 1
  "$LLVM_PROFDATA" merge -sparse "${raw[@]}" -o "$data" || return
  { printf 'TN:%s\n' "$(cov_name "$src")"; "$LLVM_COV" export -format=lcov -instr-profile="$data" "$bin"; } >"$out"
  [[ -s $out ]]
}

run_coverage() {
  need lcov; need genhtml
  local d="$ROOT/tests/coverage" raw="$ROOT/tests/coverage/coverage.raw.info"
  local project="$ROOT/tests/coverage/coverage.project.info" final="$ROOT/tests/coverage/coverage.info"
  local html="$ROOT/tests/coverage/html" desc="$ROOT/tests/coverage/test-descriptions.txt"
  local s b xml name trace profiles test_status=0 cov_status=0; local -a traces=() adds=()

  rm -rf "$d" "$ROOT/tests/test-results"; mkdir -p "$d" "$html" "$ROOT/tests/test-results" "$BIN/tests"
  : >"$desc"; for s in "${SOURCES[@]}"; do name=$(cov_name "$s"); printf 'TN:%s\nTD:%s tests\n\n' "$name" "$name" >>"$desc"; done
  find "$BIN/tests" -type f \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' \) -delete
  resolve_cov_tools; build_all coverage

  for s in "${SOURCES[@]}"; do
    b=$(bin_for "$s"); xml=$(test_xml "$s"); name=$(cov_name "$s"); trace="$d/$name.raw.info"
    mkdir -p "$(dirname "$xml")" "$(dirname "$trace")"
    if [[ $CXX_KIND == gcc ]]; then
      find "$BIN/tests" -type f -name '*.gcda' -delete
      "$b" "${ARGS[@]}" --reporters=junit --out="$xml" || test_status=1
      capture_gcc "$s" "$trace" || cov_status=1
    else
      profiles="$d/profiles/$name"; rm -rf "$profiles"; mkdir -p "$profiles"
      LLVM_PROFILE_FILE="$profiles/%p.profraw" "$b" "${ARGS[@]}" --reporters=junit --out="$xml" || test_status=1
      capture_clang "$s" "$b" "$trace" "$profiles" || cov_status=1
    fi
    [[ -s $xml ]] || test_status=1
    [[ -s $trace ]] && traces+=("$trace")
  done

  ((${#traces[@]})) || die 'no coverage tracefiles generated'
  for trace in "${traces[@]}"; do adds+=(--add-tracefile "$trace"); done
  lcov "${adds[@]}" --branch-coverage --ignore-errors inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch -o "$raw"
  lcov --extract "$raw" "$ROOT/*" --branch-coverage --ignore-errors unused,unused,inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch -o "$project"
  lcov --remove "$project" '*/third_party/*' '*/tests/*' --branch-coverage --filter brace,branch,exception \
    --ignore-errors unused,unused,inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch -o "$final"
  genhtml "$final" --function-coverage --branch-coverage --demangle-cpp --filter brace,branch,exception \
    --show-navigation --show-proportion --legend --sort --dark-mode --precision 2 \
    --title 'XTD Test Coverage' --header-title 'XTD Code Coverage' --description-file "$desc" -o "$html"
  lcov --summary "$final" --branch-coverage
  printf 'Coverage report: %s/index.html\n' "$(rel "$html")"
  ((test_status == 0 && cov_status == 0)) || die 'one or more tests or coverage captures failed'

  if [[ $ACTION == coverage-webserver ]]; then
    local py; py=$(command -v -- "$PYTHON_REQ" 2>/dev/null) || die "Python not found: $PYTHON_REQ"
    echo 'Coverage report available at: http://localhost:8000'
    exec "$py" -m http.server 8000 --directory "$html"
  fi
}

main() {
  parse "$@"; [[ -f $FLAGS ]] || die "compile flags file not found: $FLAGS"
  resolve_target; resolve_compiler; mkdir -p "$BIN"; cd "$ROOT"
  case $ACTION in
    build) build_all sample; echo "Built ${#SOURCES[@]} sample(s)" ;;
    run) compile "${SOURCES[0]}" sample; "$(bin_for "${SOURCES[0]}")" "${ARGS[@]}" ;;
    test) run_tests ;;
    benchmark) run_benchmarks ;;
    coverage|coverage-webserver) run_coverage ;;
  esac
}
main "$@"