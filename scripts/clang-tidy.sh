#!/usr/bin/env bash
# Runs clang-tidy on the given source files using the project's compile flags.
# Used by the pre-commit hook; can also be run by hand:
#   scripts/clang-tidy.sh src/*.cpp
set -euo pipefail

if [[ $# -eq 0 ]]; then
  exit 0
fi

# Find clang-tidy: $CLANG_TIDY, then PATH, then Homebrew's keg-only LLVM.
tidy="${CLANG_TIDY:-}"
if [[ -z "$tidy" ]]; then
  if command -v clang-tidy >/dev/null 2>&1; then
    tidy="clang-tidy"
  elif command -v brew >/dev/null 2>&1 &&
    [[ -x "$(brew --prefix llvm 2>/dev/null)/bin/clang-tidy" ]]; then
    tidy="$(brew --prefix llvm)/bin/clang-tidy"
  else
    echo "clang-tidy not found. See CONTRIBUTING.md for setup." >&2
    exit 1
  fi
fi

flags=(-std=c++20 -I.)
# GoogleTest headers, so tests/ can be analysed too.
if command -v pkg-config >/dev/null 2>&1; then
  # shellcheck disable=SC2207  # Intentional word splitting of compiler flags.
  flags+=($(pkg-config --cflags gtest 2>/dev/null || true))
  # shellcheck disable=SC2207
  flags+=($(pkg-config --cflags benchmark 2>/dev/null || true))
fi
if [[ "$(uname)" == "Darwin" ]]; then
  flags+=(-isysroot "$(xcrun --show-sdk-path)")
fi

"$tidy" --quiet "$@" -- "${flags[@]}"
