CXX       = g++
CXXFLAGS  = -std=c++20 -Wall -Wextra -Wpedantic -g -I.
CPPFLAGS  = -MMD -MP
LDFLAGS   =

BUILD_DIR = build
TARGET    = lob
SRCS      = src/main.cpp src/order_book.cpp
OBJS      = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS      = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)

# ---- Style (Google C++ Style Guide) -----------------------------------------
# These use the same pinned tool versions as the git hooks in
# .pre-commit-config.yaml. They only see files tracked by git, so `git add`
# new files first.

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

.PHONY: all clean setup format lint check help
