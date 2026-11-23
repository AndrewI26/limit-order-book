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
| Google Benchmark | Running the benchmarks |

`clang-format` and `cpplint` are installed by pre-commit on its own, at
pinned versions, so you don't need to install them yourself.

### macOS

```bash
xcode-select --install          # compiler + make
brew install pre-commit llvm googletest google-benchmark pkg-config   # llvm provides clang-tidy
```

Homebrew's `llvm` is keg-only, so it isn't on your `PATH`. That's fine:
`scripts/clang-tidy.sh` finds it automatically. If you want the tools on
your `PATH` anyway, add `$(brew --prefix llvm)/bin` to it.

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential clang-tidy pipx libgtest-dev libbenchmark-dev pkg-config
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

## Benchmarks

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and
live in `bench/`. They're built with optimisation (`-O3 -DNDEBUG
-march=native`) into `build/release/`, separate from the debug build:

```bash
make bench
```

Pass Google Benchmark flags through `BENCH_ARGS`:

```bash
make bench BENCH_ARGS="--benchmark_filter=MixedFlow"
make bench BENCH_ARGS="--benchmark_repetitions=10 --benchmark_report_aggregates_only=true"
make bench BENCH_ARGS="--benchmark_out=results.json"
```

For stable numbers: plug in your laptop, close heavy apps, and use
repetitions. Inputs come from fixed random seeds, so every run sees the same
data.

When adding a benchmark:

- Keep setup and random number generation outside the timed
  `for (auto _ : state)` loop.
- Wrap results in `benchmark::DoNotOptimize(...)` so the compiler can't
  remove the work.
- Keep the book's size stable across iterations (for example, pair each add
  with a cancel), or the benchmark measures a book that keeps changing.
- Use a fresh, increasing order id for every `Add`, because reused ids are
  rejected.
