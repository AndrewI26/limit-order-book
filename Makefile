CXX       = g++
CXXFLAGS  = -std=c++20 -Wall -Wextra -Wpedantic -g -I.
CPPFLAGS  = -MMD -MP
LDFLAGS   =

BUILD_DIR = build
TARGET    = lob
SRCS      = src/main.cpp src/order_book.cpp
OBJS      = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Tests (GoogleTest, found via pkg-config)
TEST_BIN  = $(BUILD_DIR)/order_book_test
TEST_SRCS = tests/order_book_test.cpp
TEST_OBJS = $(TEST_SRCS:%.cpp=$(BUILD_DIR)/%.o)
GTEST_CFLAGS := $(shell pkg-config --cflags gtest_main 2>/dev/null)
GTEST_LIBS   := $(shell pkg-config --libs gtest_main 2>/dev/null)

DEPS      = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

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

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

## setup: Install the pre-commit git hooks (run once after cloning)
setup:
	pre-commit install

## format: Reformat all C++ sources in place with clang-format
format:
	@pre-commit run clang-format --all-files || true

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

.PHONY: all clean test setup format lint check help
