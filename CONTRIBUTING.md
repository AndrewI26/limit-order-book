# Contributing

This project follows the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
Formatting and linting run automatically through
[pre-commit](https://pre-commit.com) hooks whenever you commit.

## Prerequisites

| Tool | Purpose |
| --- | --- |
| A C++20 compiler (Clang or GCC) | Building |
| `make` | Building |
| Python 3 + `pre-commit` | Running the git hooks |
| `clang-tidy` | Static analysis and naming checks |
| GoogleTest + `pkg-config` | Running the tests |

`clang-format` and `cpplint` are installed by pre-commit on its own, at
pinned versions, so you don't need to install them yourself.

### macOS

```bash
xcode-select --install          # compiler + make
brew install pre-commit llvm googletest pkg-config   # llvm provides clang-tidy
```

Homebrew's `llvm` is keg-only, so it isn't on your `PATH`. That's fine:
`scripts/clang-tidy.sh` finds it automatically. If you want the tools on
your `PATH` anyway, add `$(brew --prefix llvm)/bin` to it.

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential clang-tidy pipx libgtest-dev pkg-config
pipx install pre-commit
```

## Setup

Clone the repo and install the git hooks once:

```bash
git clone <repo-url>
cd limit-order-book
make setup
```

Optionally, run every check against the whole repo to warm the cache:

```bash
make check
```
