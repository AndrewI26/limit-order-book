#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "src/order_book.hpp"

using orderbook::OrderBook;
using orderbook::OrderId;
using orderbook::Price;
using orderbook::Qty;
using orderbook::Side;
using orderbook::Trade;

struct TestOrder {
  OrderId id;
  Side side;
  Price price;
  Qty qty;
};

static void BM_Add(benchmark::State& state) {
  constexpr int kNumOrders = 1000;
  std::vector<TestOrder> orders;
  orders.reserve(kNumOrders);

  for (int i = 0; i < kNumOrders; ++i) {
    orders.push_back({static_cast<OrderId>(i), Side::Buy, 100, 10});
  }

  for (auto _ : state) {
    OrderBook book;

    std::vector<Trade> trades;

    for (const auto& order : orders) {
      book.Add(order.id, order.side, order.price, order.qty, &trades);
    }

    benchmark::DoNotOptimize(book);
  }
  state.SetItemsProcessed(state.iterations() *
                          static_cast<int64_t>(orders.size()));
}
BENCHMARK(BM_Add);

namespace {

// ---- Helpers ----------------------------------------------------------------

// Bids rest strictly below kMid and asks strictly above it, so orders made by
// Gen never cross each other. Each side uses kLevels distinct prices, which
// clusters orders near the touch like a real book.
constexpr Price kMid = 100'000;
constexpr Price kLevels = 100;

// Deterministic (fixed-seed) generator so every run sees the same input.
class Gen {
 public:
  explicit Gen(std::uint64_t seed) : rng_(seed) {}

  Price BidPrice() { return kMid - 1 - (rng_() % kLevels); }
  Price AskPrice() { return kMid + 1 + (rng_() % kLevels); }
  Qty Quantity() { return 1 + (rng_() % 100); }
  std::uint64_t Next() { return rng_(); }

 private:
  std::mt19937_64 rng_;
};

// Adds `n` non-crossing resting orders with ids [0, n). Even ids are bids and
// odd ids are asks.
void Prefill(OrderBook& book, std::int64_t n, Gen& gen) {
  for (OrderId id = 0; id < static_cast<OrderId>(n); ++id) {
    if (id % 2 == 0) {
      book.Add(id, Side::Buy, gen.BidPrice(), gen.Quantity());
    } else {
      book.Add(id, Side::Sell, gen.AskPrice(), gen.Quantity());
    }
  }
}

// ---- Resting orders ---------------------------------------------------------

// Steady-state add: each iteration adds one resting order and cancels the
// oldest, so the book stays at exactly N orders. Times one Add + one Cancel.
// Varying N shows how cost grows as the book outgrows the CPU caches.
void BM_AddCancelSteady(benchmark::State& state) {
  const std::int64_t n = state.range(0);
  OrderBook book;
  Gen gen(1);
  Prefill(book, n, gen);

  // Pre-generate inputs so the RNG isn't timed. Even slots are bids, odd
  // slots asks; the size is even so the pattern holds across wrap-around.
  constexpr std::size_t kInputs = 65'536;
  std::vector<Price> prices(kInputs);
  std::vector<Qty> qtys(kInputs);
  for (std::size_t i = 0; i < kInputs; ++i) {
    prices[i] = i % 2 == 0 ? gen.BidPrice() : gen.AskPrice();
    qtys[i] = gen.Quantity();
  }

  auto next_id = static_cast<OrderId>(n);
  std::size_t i = 0;
  for (auto _ : state) {
    const Side side = i % 2 == 0 ? Side::Buy : Side::Sell;
    book.Add(next_id, side, prices[i], qtys[i]);
    book.Cancel(next_id - static_cast<OrderId>(n));
    ++next_id;
    i = (i + 1) % kInputs;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddCancelSteady)->RangeMultiplier(10)->Range(1'000, 1'000'000);

// Cancels an order from the middle of a single price level's queue, then
// re-adds one at the back to keep the queue length at Q. If cancel is truly
// O(1), the time should not grow with Q.
void BM_CancelFromQueueMiddle(benchmark::State& state) {
  const auto q = static_cast<OrderId>(state.range(0));
  OrderBook book;
  for (OrderId id = 0; id < q; ++id) book.Add(id, Side::Buy, kMid, 1);

  // Ids [0, q/2) are never touched; `target` always sits q/2 places from the
  // front of the queue.
  OrderId target = q / 2;
  OrderId next_id = q;
  for (auto _ : state) {
    book.Cancel(target++);
    book.Add(next_id++, Side::Buy, kMid, 1);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelFromQueueMiddle)->RangeMultiplier(10)->Range(10, 100'000);

// Partially reduces orders in place (no removal), cycling through the book.
void BM_ReducePartial(benchmark::State& state) {
  constexpr std::int64_t kOrders = 10'000;
  OrderBook book;
  for (OrderId id = 0; id < kOrders; ++id) {
    book.Add(id, Side::Buy, kMid - 1 - (id % kLevels), 1'000'000'000);
  }

  OrderId id = 0;
  for (auto _ : state) {
    book.Reduce(id, 1);
    id = (id + 1) % kOrders;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ReducePartial);

// ---- Matching ---------------------------------------------------------------

// One maker rests at the best ask, then one taker fully fills it. Times the
// rest + match pair on top of a 10k-order background book. Arg 1 also records
// fills into a Trade vector, showing what reporting trades costs.
void BM_AddFullyFilling(benchmark::State& state) {
  const bool record_trades = state.range(0) != 0;
  constexpr std::int64_t kBackground = 10'000;
  OrderBook book;
  Gen gen(2);
  Prefill(book, kBackground, gen);

  std::vector<Trade> trades;
  std::vector<Trade>* out = record_trades ? &trades : nullptr;
  OrderId next_id = kBackground;
  for (auto _ : state) {
    // kMid is inside the spread, so the sell rests as the new best ask and
    // the buy at kMid crosses only that order.
    book.Add(next_id++, Side::Sell, kMid, 10);
    book.Add(next_id++, Side::Buy, kMid, 10, out);
    trades.clear();
  }
  state.SetItemsProcessed(state.iterations());
  state.SetLabel(record_trades ? "with trades" : "no trades");
}
BENCHMARK(BM_AddFullyFilling)->Arg(0)->Arg(1);

// One aggressive order sweeps L price levels (one order each). The book is
// rebuilt with the timer paused, so only the sweep is timed. Pausing costs
// a few hundred ns, so small L is dominated by that overhead.
void BM_SweepLevels(benchmark::State& state) {
  const std::int64_t levels = state.range(0);
  OrderBook book;
  OrderId next_id = 0;
  for (auto _ : state) {
    state.PauseTiming();
    for (std::int64_t l = 0; l < levels; ++l) {
      book.Add(next_id++, Side::Sell, kMid + static_cast<Price>(l), 1);
    }
    state.ResumeTiming();

    book.Add(next_id++, Side::Buy, kMid + static_cast<Price>(levels),
             static_cast<Qty>(levels));
  }
  // Items = resting orders consumed.
  state.SetItemsProcessed(state.iterations() * levels);
}
BENCHMARK(BM_SweepLevels)->RangeMultiplier(10)->Range(1, 1'000);

// ---- Queries ----------------------------------------------------------------

void BM_BestBid(benchmark::State& state) {
  OrderBook book;
  Gen gen(3);
  Prefill(book, 10'000, gen);
  for (auto _ : state) benchmark::DoNotOptimize(book.BestBid());
}
BENCHMARK(BM_BestBid);

void BM_Front(benchmark::State& state) {
  OrderBook book;
  Gen gen(3);
  Prefill(book, 10'000, gen);
  for (auto _ : state) benchmark::DoNotOptimize(book.Front(Side::Buy));
}
BENCHMARK(BM_Front);

// Looks up random prices that exist on the bid side.
void BM_VolumeAt(benchmark::State& state) {
  OrderBook book;
  Gen gen(4);
  Prefill(book, 10'000, gen);

  constexpr std::size_t kQueries = 1'024;
  std::vector<Price> queries(kQueries);
  for (auto& price : queries) price = gen.BidPrice();

  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(book.VolumeAt(Side::Buy, queries[i]));
    i = (i + 1) % kQueries;
  }
}
BENCHMARK(BM_VolumeAt);

// Returns the top K of 1,000 bid levels. Cost should grow with K (building the
// result vector), not with the total number of levels.
void BM_Depth(benchmark::State& state) {
  constexpr OrderId kTotalLevels = 1'000;
  OrderBook book;
  for (OrderId id = 0; id < kTotalLevels; ++id) {
    book.Add(id, Side::Buy, kMid - 1 - id, 1);
  }

  const auto k = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) benchmark::DoNotOptimize(book.Depth(Side::Buy, k));
}
BENCHMARK(BM_Depth)->Arg(1)->Arg(10)->Arg(100);

// ---- Realistic flow ---------------------------------------------------------

enum class OpType : std::uint8_t { Add, Cancel, Cross };

struct Op {
  OpType type;
  OrderId id;
  Side side;
  Price price;
  Qty qty;
};

// Builds a fixed sequence of operations against a book prefilled with ids
// [0, prefill): ~50% resting adds, ~45% cancels of random live orders, ~5%
// aggressive orders that trade. Adds and removals roughly balance so the book
// size stays stable. Some cancels target orders that were already filled;
// those return false, as they would in a real feed.
std::vector<Op> MakeMixedFlow(std::size_t count, std::int64_t prefill,
                              Gen& gen) {
  std::vector<OrderId> live;
  live.reserve(static_cast<std::size_t>(prefill) + count);
  for (OrderId id = 0; id < static_cast<OrderId>(prefill); ++id) {
    live.push_back(id);
  }

  std::vector<Op> ops;
  ops.reserve(count);
  auto next_id = static_cast<OrderId>(prefill);
  for (std::size_t i = 0; i < count; ++i) {
    const std::uint64_t roll = gen.Next() % 100;
    const Side side = gen.Next() % 2 == 0 ? Side::Buy : Side::Sell;
    if (roll < 50 || live.empty()) {
      const Price price = side == Side::Buy ? gen.BidPrice() : gen.AskPrice();
      ops.push_back({OpType::Add, next_id, side, price, gen.Quantity()});
      live.push_back(next_id++);
    } else if (roll < 95) {
      const std::size_t pick = gen.Next() % live.size();
      ops.push_back({OpType::Cancel, live[pick], side, 0, 0});
      live[pick] = live.back();
      live.pop_back();
    } else {
      // Priced through the far end of the opposite side, so it always trades.
      const Price price = side == Side::Buy ? kMid + kLevels : kMid - kLevels;
      ops.push_back({OpType::Cross, next_id++, side, price, gen.Quantity()});
    }
  }
  return ops;
}

// Replays the mixed flow against a fresh 10k-order book each iteration. This
// is the closest thing here to a headline throughput number.
void BM_MixedFlow(benchmark::State& state) {
  constexpr std::int64_t kPrefill = 10'000;
  constexpr std::size_t kOps = 100'000;
  Gen gen(5);
  const std::vector<Op> ops = MakeMixedFlow(kOps, kPrefill, gen);

  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    Gen prefill_gen(6);
    Prefill(book, kPrefill, prefill_gen);
    state.ResumeTiming();

    for (const Op& op : ops) {
      if (op.type == OpType::Cancel) {
        book.Cancel(op.id);
      } else {
        book.Add(op.id, op.side, op.price, op.qty);
      }
    }
    benchmark::ClobberMemory();

    // Destroying the book isn't part of the flow being measured.
    state.PauseTiming();
    book = OrderBook();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kOps));
}
BENCHMARK(BM_MixedFlow)->Unit(benchmark::kMillisecond);

}  // namespace

BENCHMARK_MAIN();
