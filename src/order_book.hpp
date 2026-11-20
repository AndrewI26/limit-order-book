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
  // Add a resting limit order at the back of its price level's queue.
  // Returns false (and changes nothing) if `id` already exists or qty <= 0.
  bool Add(const OrderId id, const Side side, const Price price, const Qty qty);

  // Remove an order entirely. Returns false if `id` is unknown.
  bool Cancel(const OrderId id);

  // Reduce an order's quantity by `by`. It keeps its place in the queue.
  // Returns false if `id` is unknown or by <= 0. If its quantity reaches 0, it
  // is removed.
  bool Reduce(const OrderId id, const Qty by);

  std::optional<Level> BestBid() const;
  std::optional<Level> BestAsk() const;

  // Next order to be filled.
  std::optional<OrderId> Front(const Side side) const;
  Qty VolumeAt(const Side side, const Price price) const;
  std::vector<Level> Depth(const Side side,
                           const std::size_t n) const;  // best first
  std::size_t OrderCount() const;

  friend std::ostream& operator<<(std::ostream& out, const OrderBook& ob);

 private:
  // Unlinks the order at `it` from its level, updating the level's totals and
  // erasing the level if it becomes empty. Does not touch `orders_`.
  void RemoveFromLevel(LevelQueue::iterator it);

  // Stores the levels
  std::map<Price, Level, std::greater<Price>> bids_;
  std::map<Price, Level> asks_;

  std::unordered_map<OrderId, OrderLocation> orders_;
};

}  // namespace orderbook
