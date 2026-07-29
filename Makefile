SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.DELETE_ON_ERROR:

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BIN  := $(ROOT)/bin

# GNU Make defaults CXX to g++; override with: make CXX=clang++
FLAGS_FILE    := $(ROOT)/compile_flags.txt
MAKEFILE_SELF := $(lastword $(MAKEFILE_LIST))

COMMON := -pthread -MMD -MP
WARNINGS := \
	-Werror -Wpedantic -pedantic-errors \
	-Wconversion -Wsign-conversion -Wshadow \
	-Wformat=2 -Wundef -Wcast-align -Wcast-qual \
	-Wold-style-cast -Woverloaded-virtual \
	-Wnon-virtual-dtor -Wnull-dereference \
	-Wdouble-promotion -Wswitch-enum

CXX_VERSION       := $(shell $(CXX) --version 2>/dev/null | head -1)
CXX_VERSION_LOWER := $(shell printf '%s' '$(CXX_VERSION)' | tr '[:upper:]' '[:lower:]')
CXX_MAJOR         := $(shell $(CXX) -dumpversion 2>/dev/null | sed 's/\..*//')

CATCH_DIR   := $(ROOT)/tests/third_party/catch2
CATCH_HPP   := $(CATCH_DIR)/catch_amalgamated.hpp
CATCH_SRC   := $(CATCH_DIR)/catch_amalgamated.cpp
CATCH_OBJ   := $(BIN)/third_party/catch2/catch_amalgamated.o
CATCH_DEP   := $(CATCH_OBJ:.o=.d)
CATCH_FLAGS := -I$(CATCH_DIR)

ifneq ($(findstring clang,$(CXX_VERSION_LOWER)),)

CXX_KIND := clang

WARNINGS += \
	-Wextra-semi \
	-Wimplicit-fallthrough \
	-Wnewline-eof \
	-Wzero-as-null-pointer-constant

COVERAGE_FLAGS := \
	-O0 \
	-g \
	-fprofile-instr-generate \
	-fcoverage-mapping

LLVM_COV ?= $(shell \
	for tool in \
		"$$(dirname "$$(command -v $(CXX) 2>/dev/null)")/llvm-cov" \
		"llvm-cov-$(CXX_MAJOR)" \
		"/usr/lib/llvm-$(CXX_MAJOR)/bin/llvm-cov" \
		llvm-cov; \
	do \
		command -v "$$tool" 2>/dev/null && break; \
	done)

LLVM_PROFDATA ?= $(shell \
	for tool in \
		"$$(dirname "$$(command -v $(CXX) 2>/dev/null)")/llvm-profdata" \
		"llvm-profdata-$(CXX_MAJOR)" \
		"/usr/lib/llvm-$(CXX_MAJOR)/bin/llvm-profdata" \
		llvm-profdata; \
	do \
		command -v "$$tool" 2>/dev/null && break; \
	done)

else ifneq ($(findstring gcc,$(CXX_VERSION_LOWER))$(findstring g++,$(CXX_VERSION_LOWER)),)

CXX_KIND := gcc

WARNINGS += \
	-Wduplicated-cond \
	-Wduplicated-branches \
	-Wlogical-op \
	-Wuseless-cast

COVERAGE_FLAGS := \
	-O0 \
	-g \
	--coverage

GCOV ?= $(shell \
	tool="$$($(CXX) -print-prog-name=gcov 2>/dev/null)"; \
	command -v "$$tool" 2>/dev/null || command -v gcov 2>/dev/null)

else

$(error Unsupported compiler: $(CXX_VERSION))

endif

BUILD_CONFIG := $(BIN)/.build-config

SAMPLE_SRCS := $(shell \
	find samples \
		-type f \
		-name '*.cpp' \
		-print 2>/dev/null | \
	LC_ALL=C sort)

TEST_SRCS := $(shell \
	find tests \
		\( \
			-path 'tests/third_party' -o \
			-path 'tests/coverage' -o \
			-path 'tests/test-results' \
		\) -prune -o \
		-type f \
		-name '*.cpp' \
		-print 2>/dev/null | \
	LC_ALL=C sort)

BENCHMARK_SRCS := $(shell \
	find benchmarks \
		-type f \
		-name '*.cpp' \
		-print 2>/dev/null | \
	LC_ALL=C sort)

SAMPLE_BINS    := $(patsubst %.cpp,$(BIN)/%,$(SAMPLE_SRCS))
TEST_BINS      := $(patsubst %.cpp,$(BIN)/%,$(TEST_SRCS))
BENCHMARK_BINS := $(patsubst %.cpp,$(BIN)/%,$(BENCHMARK_SRCS))

COVERAGE_BIN  := $(BIN)/coverage
COVERAGE_DIR  := $(ROOT)/tests/coverage
COVERAGE_HTML := $(COVERAGE_DIR)/html
TEST_RESULTS  := $(ROOT)/tests/test-results

COVERAGE_BINS := $(patsubst %.cpp,$(COVERAGE_BIN)/%,$(TEST_SRCS))

ALL_BINS := \
	$(SAMPLE_BINS) \
	$(TEST_BINS) \
	$(BENCHMARK_BINS) \
	$(COVERAGE_BINS)

DEPS := $(ALL_BINS:%=%.d) $(CATCH_DEP)

TEST_RUN_SRCS := $(if $(strip $(FILE)),$(FILE),$(TEST_SRCS))
TEST_RUN_BINS := $(patsubst %.cpp,$(BIN)/%,$(TEST_RUN_SRCS))

BENCHMARK_RUN_SRCS := $(if $(strip $(FILE)),$(FILE),$(BENCHMARK_SRCS))
BENCHMARK_RUN_BINS := $(patsubst %.cpp,$(BIN)/%,$(BENCHMARK_RUN_SRCS))

COVERAGE_RUN_SRCS := $(if $(strip $(FILE)),$(FILE),$(TEST_SRCS))
COVERAGE_RUN_BINS := $(patsubst %.cpp,$(COVERAGE_BIN)/%,$(COVERAGE_RUN_SRCS))

ifneq ($(filter run,$(MAKECMDGOALS)),)

ifeq ($(strip $(FILE)),)
$(error FILE is required, for example: make run FILE=samples/example.cpp)
endif

ifeq ($(filter samples/%.cpp,$(FILE)),)
$(error FILE must be a repository-relative samples/*.cpp path)
endif

ifeq ($(wildcard $(FILE)),)
$(error FILE not found: $(FILE))
endif

endif

ifneq ($(filter test,$(MAKECMDGOALS)),)

ifneq ($(strip $(FILE)),)

ifeq ($(filter tests/%.cpp,$(FILE)),)
$(error FILE must be a repository-relative tests/*.cpp path)
endif

ifeq ($(wildcard $(FILE)),)
$(error FILE not found: $(FILE))
endif

endif

endif

ifneq ($(filter coverage,$(MAKECMDGOALS)),)

ifneq ($(strip $(FILE)),)

ifeq ($(filter tests/%.cpp,$(FILE)),)
$(error FILE must be a repository-relative tests/*.cpp path)
endif

ifeq ($(wildcard $(FILE)),)
$(error FILE not found: $(FILE))
endif

endif

endif

ifneq ($(filter benchmark,$(MAKECMDGOALS)),)

ifneq ($(strip $(FILE)),)

ifeq ($(filter benchmarks/%.cpp,$(FILE)),)
$(error FILE must be a repository-relative benchmarks/*.cpp path)
endif

ifeq ($(wildcard $(FILE)),)
$(error FILE not found: $(FILE))
endif

endif

endif

.PHONY: \
	all \
	samples \
	tests \
	coverages \
	benchmarks \
	run \
	test \
	benchmark \
	coverage \
	check-coverage-tools \
	clean \
	help \
	FORCE

FORCE:

$(BUILD_CONFIG): FORCE
	@mkdir -p "$(@D)"

	tmp="$$(mktemp "$(BIN)/.build-config.XXXXXX")"

	printf '%s\n' \
		'CXX=$(CXX)' \
		'CXX_VERSION=$(CXX_VERSION)' \
		'COMMON=$(COMMON)' \
		'WARNINGS=$(WARNINGS)' \
		'CATCH_FLAGS=$(CATCH_FLAGS)' \
		'COVERAGE_FLAGS=$(COVERAGE_FLAGS)' >"$$tmp"

	if [[ ! -f "$@" ]] || ! cmp -s "$$tmp" "$@"; then
		mv "$$tmp" "$@"
	else
		rm -f "$$tmp"
	fi

all: samples tests coverages benchmarks

samples: $(SAMPLE_BINS)

tests: $(TEST_BINS)

coverages: $(COVERAGE_BINS)

benchmarks: $(BENCHMARK_BINS)

$(BIN)/samples/%: samples/%.cpp $(FLAGS_FILE) $(MAKEFILE_SELF) $(BUILD_CONFIG)
	@mkdir -p "$(@D)"

	echo "[$(CXX)] Building $< -> $(patsubst $(ROOT)/%,%,$@)"

	$(CXX) \
		@$(FLAGS_FILE) \
		"$<" \
		-o "$@" \
		$(COMMON)

$(CATCH_OBJ): $(CATCH_SRC) $(CATCH_HPP) $(FLAGS_FILE) $(MAKEFILE_SELF) $(BUILD_CONFIG)
	@mkdir -p "$(@D)"

	echo "[$(CXX)] Building Catch2 -> $(patsubst $(ROOT)/%,%,$@)"

	$(CXX) \
		@$(FLAGS_FILE) \
		$(CATCH_FLAGS) \
		"$<" \
		-c \
		-o "$@" \
		$(COMMON) \
		-O3

$(BIN)/tests/%: tests/%.cpp $(CATCH_OBJ) $(FLAGS_FILE) $(MAKEFILE_SELF) $(BUILD_CONFIG)
	@mkdir -p "$(@D)"

	echo "[$(CXX)] Building $< -> $(patsubst $(ROOT)/%,%,$@)"

	$(CXX) \
		@$(FLAGS_FILE) \
		$(CATCH_FLAGS) \
		"$<" \
		"$(CATCH_OBJ)" \
		-o "$@" \
		$(COMMON) \
		$(WARNINGS) \
		-O3

$(BIN)/benchmarks/%: benchmarks/%.cpp $(FLAGS_FILE) $(MAKEFILE_SELF) $(BUILD_CONFIG)
	@mkdir -p "$(@D)"

	echo "[$(CXX)] Building $< -> $(patsubst $(ROOT)/%,%,$@)"

	$(CXX) \
		@$(FLAGS_FILE) \
		"$<" \
		-o "$@" \
		$(COMMON) \
		-O3

$(COVERAGE_BIN)/tests/%: tests/%.cpp $(CATCH_OBJ) $(FLAGS_FILE) $(MAKEFILE_SELF) $(BUILD_CONFIG)
	@mkdir -p "$(@D)"

	echo "[$(CXX)] Building coverage $< -> $(patsubst $(ROOT)/%,%,$@)"

	$(CXX) \
		@$(FLAGS_FILE) \
		$(CATCH_FLAGS) \
		"$<" \
		"$(CATCH_OBJ)" \
		-o "$@" \
		$(COMMON) \
		$(WARNINGS) \
		$(COVERAGE_FLAGS)

run: $(patsubst %.cpp,$(BIN)/%,$(FILE))
	@"$(patsubst %.cpp,$(BIN)/%,$(FILE))" $(ARGS)

test: $(TEST_RUN_BINS)
	@if [[ -n "$(strip $(FILE))" ]]; then
		name="$(FILE)"
		name="$${name#tests/}"

		rm -f "$(TEST_RESULTS)/$${name%.cpp}.xml"
	else
		rm -rf "$(TEST_RESULTS)"
	fi

	mkdir -p "$(TEST_RESULTS)"

	status=0

	for binary in $(TEST_RUN_BINS); do
		name="$${binary#$(BIN)/tests/}"
		xml="$(TEST_RESULTS)/$$name.xml"

		mkdir -p "$$(dirname "$$xml")"

		echo "Running $${binary#$(ROOT)/}"

		"$$binary" $(ARGS) \
			--reporter console::out=- \
			--reporter junit::out="$$xml" || status=1

		[[ -s "$$xml" ]] || {
			echo "JUnit report missing: $$xml" >&2
			status=1
		}
	done

	exit $$status

benchmark: $(BENCHMARK_RUN_BINS)
	@status=0

	for binary in $(BENCHMARK_RUN_BINS); do
		name="$${binary#$(BIN)/benchmarks/}"
		output="$(ROOT)/benchmarks/results/$$name.md"

		mkdir -p "$$(dirname "$$output")"

		echo "Running $${binary#$(ROOT)/}"

		"$$binary" $(ARGS) 2>&1 | tee "$$output" || status=1
	done

	exit $$status

check-coverage-tools:
	@command -v lcov >/dev/null 2>&1 || {
		echo 'error: lcov not found' >&2
		exit 1
	}

	command -v genhtml >/dev/null 2>&1 || {
		echo 'error: genhtml not found' >&2
		exit 1
	}

	if [[ "$(CXX_KIND)" == clang ]]; then
		[[ -n "$(LLVM_COV)" ]] &&
			command -v "$(LLVM_COV)" >/dev/null 2>&1 || {
				echo 'error: llvm-cov not found' >&2
				exit 1
			}

		[[ -n "$(LLVM_PROFDATA)" ]] &&
			command -v "$(LLVM_PROFDATA)" >/dev/null 2>&1 || {
				echo 'error: llvm-profdata not found' >&2
				exit 1
			}
	else
		[[ -n "$(GCOV)" ]] &&
			command -v "$(GCOV)" >/dev/null 2>&1 || {
				echo 'error: gcov not found' >&2
				exit 1
			}
	fi

define RUN_COVERAGE
@raw="$(COVERAGE_DIR)/coverage.raw.info"
project="$(COVERAGE_DIR)/coverage.project.info"
final="$(COVERAGE_DIR)/coverage.info"
desc="$(COVERAGE_DIR)/test-descriptions.txt"

status=0
traces=()

rm -rf \
	"$(COVERAGE_DIR)" \
	"$(TEST_RESULTS)"

mkdir -p \
	"$(COVERAGE_HTML)" \
	"$(TEST_RESULTS)"

find "$(COVERAGE_BIN)/tests" \
	-type f \
	\( -name '*.gcda' -o -name '*.gcov' \) \
	-delete 2>/dev/null || true

: >"$$desc"

for source in $(1); do
	name="$${source#tests/}"
	name="$${name%.cpp}"

	printf 'TN:%s\nTD:%s tests\n\n' \
		"$$name" \
		"$$name" >>"$$desc"
done

for binary in $(2); do
	name="$${binary#$(COVERAGE_BIN)/tests/}"
	xml="$(TEST_RESULTS)/$$name.xml"
	trace="$(COVERAGE_DIR)/traces/$$name.info"

	mkdir -p \
		"$$(dirname "$$xml")" \
		"$$(dirname "$$trace")"

	echo "Running coverage: $${binary#$(ROOT)/}"

	if [[ "$(CXX_KIND)" == gcc ]]; then
		find "$(COVERAGE_BIN)/tests" \
			-type f \
			-name '*.gcda' \
			-delete 2>/dev/null || true

		"$$binary" $(ARGS) \
			--reporter console::out=- \
			--reporter junit::out="$$xml" || status=1

		lcov \
			--capture \
			--directory "$(COVERAGE_BIN)/tests" \
			--base-directory "$(ROOT)" \
			--no-external \
			--branch-coverage \
			--test-name "$$name" \
			--gcov-tool "$(GCOV)" \
			--ignore-errors inconsistent,mismatch \
			--output-file "$$trace" || status=1
	else
		profiles="$(COVERAGE_DIR)/profiles/$$name"
		profdata="$(COVERAGE_DIR)/profdata/$$name.profdata"

		rm -rf "$$profiles"

		mkdir -p \
			"$$profiles" \
			"$$(dirname "$$profdata")"

		LLVM_PROFILE_FILE="$$profiles/%p.profraw" \
			"$$binary" $(ARGS) \
				--reporter console::out=- \
				--reporter junit::out="$$xml" || status=1

		mapfile -d '' profiles_raw < <(
			find "$$profiles" \
				-type f \
				-name '*.profraw' \
				-print0 |
			sort -z
		)

		if (( $${#profiles_raw[@]} == 0 )); then
			echo "No Clang profiles generated for $$name" >&2
			status=1
		else
			"$(LLVM_PROFDATA)" merge \
				-sparse \
				"$${profiles_raw[@]}" \
				-o "$$profdata" || status=1

			{
				printf 'TN:%s\n' "$$name"

				"$(LLVM_COV)" export \
					-format=lcov \
					-instr-profile="$$profdata" \
					"$$binary"
			} >"$$trace" || status=1
		fi
	fi

	[[ -s "$$xml" ]] || {
		echo "JUnit report missing: $$xml" >&2
		status=1
	}

	[[ -s "$$trace" ]] && traces+=("$$trace") || {
		echo "Coverage trace missing: $$trace" >&2
		status=1
	}
done

(( $${#traces[@]} > 0 )) || {
	echo 'error: no coverage tracefiles generated' >&2
	exit 1
}

adds=()

for trace in "$${traces[@]}"; do
	adds+=(--add-tracefile "$$trace")
done

lcov \
	"$${adds[@]}" \
	--branch-coverage \
	--ignore-errors inconsistent,corrupt,mismatch \
	--output-file "$$raw"

lcov \
	--extract "$$raw" \
	"$(ROOT)/*" \
	--branch-coverage \
	--ignore-errors unused,inconsistent,corrupt,mismatch \
	--output-file "$$project"

lcov \
	--remove "$$project" \
	'*/third_party/*' \
	'*/tests/*' \
	--branch-coverage \
	--ignore-errors unused,inconsistent,corrupt,mismatch \
	--output-file "$$final"

genhtml "$$final" \
	--function-coverage \
	--branch-coverage \
	--demangle-cpp \
	--legend \
	--sort \
	--title 'XTD Test Coverage' \
	--header-title 'XTD Code Coverage' \
	--description-file "$$desc" \
	--output-directory "$(COVERAGE_HTML)"

lcov \
	--summary "$$final" \
	--branch-coverage

echo "Coverage report: tests/coverage/html/index.html"

(( status == 0 )) || {
	echo 'error: one or more tests or coverage captures failed' >&2
	exit 1
}
endef

coverage: check-coverage-tools $(COVERAGE_RUN_BINS)
	$(call RUN_COVERAGE,$(COVERAGE_RUN_SRCS),$(COVERAGE_RUN_BINS))

clean:
	@rm -rf \
		"$(BIN)" \
		"$(TEST_RESULTS)" \
		"$(COVERAGE_DIR)" \
		"$(ROOT)/benchmarks/results"

help:
	@cat <<-'TXT'
	Targets:
	make all                                                    # Build all sample, test, coverage, and benchmark binaries
	make samples                                                # Build all sample binaries
	make tests                                                  # Build all test binaries
	make coverages                                              # Build all coverage binaries
	make benchmarks                                             # Build all benchmark binaries
	make run FILE=samples/example.cpp ARGS="..."                # Build if needed, then run one sample
	make test [FILE=tests/example.cpp] [ARGS="..."]             # Build if needed, then run one or all tests
	make benchmark [FILE=benchmarks/example.cpp] [ARGS="..."]   # Build if needed, then run one or all benchmarks
	make coverage [FILE=tests/example.cpp] [ARGS="..."]         # Build if needed, then run coverage for one or all tests
	make clean                                                  # Remove generated binaries and reports

	Overrides:
	  CXX=clang++
	  CXX=g++
	  LLVM_COV=/path/to/llvm-cov
	  LLVM_PROFDATA=/path/to/llvm-profdata
	  GCOV=/path/to/gcov
	TXT

-include $(DEPS)