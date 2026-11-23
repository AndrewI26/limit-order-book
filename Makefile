CXX       = g++
CXXFLAGS  = -std=c++20 -Wall -Wextra -Wpedantic -g -I.
CPPFLAGS  = -MMD -MP
LDFLAGS   =

BUILD_DIR = build
TARGET    = lob
SRCS      = src/main.cpp src/order_book.cpp
OBJS      = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

TEST_BIN  = $(BUILD_DIR)/order_book_test
TEST_SRCS = tests/order_book_test.cpp
TEST_OBJS = $(TEST_SRCS:%.cpp=$(BUILD_DIR)/%.o)
GTEST_CFLAGS := $(shell pkg-config --cflags gtest_main 2>/dev/null)
GTEST_LIBS   := $(shell pkg-config --libs gtest_main 2>/dev/null)

RELEASE_DIR      = $(BUILD_DIR)/release
ARCH_FLAGS      ?= -march=native
RELEASE_CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O3 -DNDEBUG \
                   $(ARCH_FLAGS) -I.
RELEASE_OBJS     = $(SRCS:%.cpp=$(RELEASE_DIR)/%.o)
RELEASE_BIN      = $(RELEASE_DIR)/$(TARGET)

BENCH_BIN    = $(RELEASE_DIR)/order_book_bench
BENCH_SRCS   = bench/order_book_bench.cpp
BENCH_OBJS   = $(BENCH_SRCS:%.cpp=$(RELEASE_DIR)/%.o)
BENCH_CFLAGS := $(shell pkg-config --cflags benchmark 2>/dev/null)
BENCH_LIBS   := $(shell pkg-config --libs benchmark 2>/dev/null)
BENCH_ARGS  ?=

DEPS      = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(RELEASE_OBJS:.o=.d) \
            $(BENCH_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

## release: Build an optimised ./build/release/lob (-O3, for benchmarking)
release: $(RELEASE_BIN)

$(RELEASE_BIN): $(RELEASE_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(RELEASE_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(RELEASE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJS): CXXFLAGS += $(GTEST_CFLAGS)

$(TEST_BIN): $(TEST_OBJS) $(BUILD_DIR)/src/order_book.o
	@if [ -z "$(GTEST_LIBS)" ]; then \
	  echo "GoogleTest not found via pkg-config. See CONTRIBUTING.md." >&2; \
	  exit 1; \
	fi
	$(CXX) $(LDFLAGS) -o $@ $^ $(GTEST_LIBS)

## test: Build and run the GoogleTest suite
test: $(TEST_BIN)
	./$(TEST_BIN)

# Benchmarks are always built with the optimised release flags.
$(BENCH_OBJS): RELEASE_CXXFLAGS += $(BENCH_CFLAGS)

$(BENCH_BIN): $(BENCH_OBJS) $(RELEASE_DIR)/src/order_book.o
	@if [ -z "$(BENCH_LIBS)" ]; then \
	  echo "Google Benchmark not found via pkg-config. See CONTRIBUTING.md." >&2; \
	  exit 1; \
	fi
	$(CXX) $(LDFLAGS) -o $@ $^ $(BENCH_LIBS)

## bench: Build (optimised) and run the benchmarks; pass flags
#   with BENCH_ARGS, e.g. make bench BENCH_ARGS=--benchmark_filter=Add
bench: $(BENCH_BIN)
	./$(BENCH_BIN) $(BENCH_ARGS)

## bench-save: Run benchmarks (10 reps) and save to bench/results/NAME.json
BENCH_RESULTS_DIR = bench/results
BENCH_REPS       ?= 10
bench-save: $(BENCH_BIN)
	@if [ -z "$(NAME)" ]; then \
	  echo "Usage: make bench-save NAME=v1" >&2; exit 1; \
	fi
	@if [ -e "$(BENCH_RESULTS_DIR)/$(NAME).json" ] && [ -z "$(FORCE)" ]; then \
	  echo "$(BENCH_RESULTS_DIR)/$(NAME).json exists; use FORCE=1 to overwrite." >&2; \
	  exit 1; \
	fi
	@mkdir -p $(BENCH_RESULTS_DIR)
	./$(BENCH_BIN) --benchmark_repetitions=$(BENCH_REPS) \
	  --benchmark_report_aggregates_only=true \
	  --benchmark_out=$(BENCH_RESULTS_DIR)/$(NAME).json \
	  --benchmark_out_format=json $(BENCH_ARGS)
	@echo "Saved $(BENCH_RESULTS_DIR)/$(NAME).json"

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

## setup: Install the pre-commit git hooks (run once after cloning)
setup:
	pre-commit install

## format: Reformat all C++ sources in place with clang-format
# Passes every C++ file git knows about plus untracked ones (minus anything
# gitignored), so new files are formatted before you `git add` them.
CXX_FILES = $(shell git ls-files --cached --others --exclude-standard \
              '*.cpp' '*.hpp' '*.cc' '*.h')
format:
	@pre-commit run clang-format --files $(CXX_FILES) || true

## lint: Run cpplint and clang-tidy
lint:
	pre-commit run cpplint --all-files
	pre-commit run clang-tidy --all-files

## check: Run every pre-commit hook (formatting, linting, whitespace, ...)
check:
	pre-commit run --all-files

## help: List available targets
help:
	@echo "Targets:"
	@echo "  all      Build ./$(TARGET) (default)"
	@echo "  clean    Remove build artifacts"
	@grep -hE '^## ' $(firstword $(MAKEFILE_LIST)) | sed -E 's/^## ([a-z-]+): /  \1\t/' | expand -t 11

.PHONY: all release clean test bench bench-save setup format lint check help
