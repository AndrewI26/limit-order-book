#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace orderbook {

enum class Side : std::uint8_t { Buy, Sell };

using OrderId = std::uint64_t;
using Price = std::uint64_t;
using Qty = std::uint64_t;

struct Order {
  OrderId id;
  Side side;
  Price price;
  Qty qty;
};
// A fill between an incoming (taker) order and a resting (maker) order.
struct Trade {
  OrderId taker_id;
  OrderId maker_id;
  Price price;  // Always the maker's (resting) price.
  Qty qty;
  bool operator==(const Trade&) const = default;
};

using LevelQueue = std::list<Order>;
struct OrderLocation {
  LevelQueue::iterator it;
  LevelQueue* orders;
};

struct Level {
  LevelQueue orders;
  Qty qty;              // Total resting quantity at this price.
  std::uint32_t count;  // Number of orders at this price.
  bool operator==(const Level&) const = default;
};

class OrderBook {
 public:
  // Submit a limit order. It first matches against the opposite side of the
  // book while prices cross (a buy at or above the best ask, a sell at or
  // below the best bid), filling resting orders in price-time priority at
  // their resting price. Any remaining quantity rests at the back of its price
  // level's queue.
  //
  // If `trades` is non-null, each fill is appended to it in execution order.
  // Returns false (and changes nothing) if `id` is already resting or
  // qty <= 0. An order that fills completely never rests, so its id is not
  // retained.
  bool Add(const OrderId id, const Side side, const Price price, const Qty qty,
           std::vector<Trade>* trades = nullptr);

  // Remove an order entirely. Returns false if `id` is unknown.
  bool Cancel(const OrderId id);

  // Reduce an order's quantity by `by`. It keeps its place in the queue.
  // Returns false if `id` is unknown or by <= 0. If its quantity reaches 0, it
  // is removed.
  bool Reduce(const OrderId id, const Qty by);

  // The best (highest) bid / best (lowest) ask level, or nullptr if that side
  // is empty. These point into the book: they are invalidated by any call that
  // modifies it (Add, Cancel, Reduce).
  const Level* BestBid() const;
  const Level* BestAsk() const;

  // Next order to be filled.
  std::optional<OrderId> Front(const Side side) const;
  Qty VolumeAt(const Side side, const Price price) const;

  // Up to `n` levels on `side`, best price first. Like BestBid/BestAsk, the
  // pointers are invalidated by any modifying call.
  std::vector<const Level*> Depth(const Side side, const std::size_t n) const;
  std::size_t OrderCount() const;

  friend std::ostream& operator<<(std::ostream& out, const OrderBook& ob);

 private:
  // Fills `id` against `levels` (the opposite side) while `crosses(level
  // price)` holds. Returns the quantity left unfilled.
  template <typename Levels, typename Crosses>
  Qty Match(OrderId id, Qty qty, Levels& levels, Crosses crosses,
            std::vector<Trade>* trades);

  // Unlinks the order at `it` from its level, updating the level's totals and
  // erasing the level if it becomes empty. Does not touch `orders_`.
  void RemoveFromLevel(LevelQueue::iterator it);

  // Stores the levels
  std::map<Price, Level, std::greater<Price>> bids_;
  std::map<Price, Level> asks_;

  std::unordered_map<OrderId, OrderLocation> orders_;
};

}  // namespace orderbook
