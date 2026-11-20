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

`clang-format` and `cpplint` are installed by pre-commit on its own, at
pinned versions, so you don't need to install them yourself.

### macOS

```bash
xcode-select --install          # compiler + make
brew install pre-commit llvm    # llvm provides clang-tidy
```

Homebrew's `llvm` is keg-only, so it isn't on your `PATH`. That's fine:
`scripts/clang-tidy.sh` finds it automatically. If you want the tools on
your `PATH` anyway, add `$(brew --prefix llvm)/bin` to it.

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential clang-tidy pipx
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

## Make targets

| Command | What it does |
| --- | --- |
| `make` | Build `./lob` |
| `make clean` | Remove build artifacts |
| `make setup` | Install the pre-commit git hooks (once after cloning) |
| `make format` | Reformat all C++ sources in place with clang-format |
| `make lint` | Run cpplint and clang-tidy |
| `make check` | Run every hook: formatting, linting, whitespace, YAML |
| `make help` | List all targets |

The style targets run through pre-commit, so they use the same pinned tool
versions as the git hooks. They only see files tracked by git, so
`git add` a new file before formatting or linting it.

## Style tooling

| Tool | Config | What it does |
| --- | --- | --- |
| clang-format | `.clang-format` | Formats code to Google style (`BasedOnStyle: Google`). Rewrites files in place. |
| cpplint | `CPPLINT.cfg` | Google's own style linter (includes, line length, casts, and similar). |
| clang-tidy | `.clang-tidy` | Google naming conventions, `google-*`, `bugprone-*` and `performance-*` checks. Warnings are treated as errors. |
| pre-commit-hooks | `.pre-commit-config.yaml` | Trailing whitespace, final newlines, YAML validity, merge markers. |

### Naming quick reference

| Kind | Style | Example |
| --- | --- | --- |
| Types (class, struct, enum, alias) | `CamelCase` | `OrderBook`, `Price` |
| Functions and methods | `CamelCase` | `BestBid()` |
| Variables and parameters | `snake_case` | `order_id` |
| Private and protected members | `snake_case_` | `bids_` |
| Enumerators, constants | `kCamelCase` | `Side::kBuy` |
| Namespaces | `snake_case` | `orderbook` |
| Macros | `UPPER_CASE` | `LOB_ASSERT` |

## Day-to-day workflow

- **Commit as usual.** The hooks run on staged files. If clang-format or a
  whitespace hook changes a file, the commit stops. Review the changes,
  `git add` them, and commit again.
- **Before pushing:** run `make format` and then `make check`.
- **Update hook versions:** `pre-commit autoupdate`
- **Skipping hooks** (`git commit --no-verify`) is for emergencies only.
  Reviewers still expect clean code.

## Editor integration

Most editors pick up `.clang-format` and `.clang-tidy` automatically:

- **VS Code:** install the *clangd* extension and turn on format on save.
- **CLion:** both config files are detected out of the box.
