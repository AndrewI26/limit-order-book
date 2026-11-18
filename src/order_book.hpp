#pragma once

#include <cstdint>
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
using OrderLocation = LevelQueue::iterator;

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
  bool add(const OrderId id, const Side side, const Price price, const Qty qty);

  // Remove an order entirely. Returns false if `id` is unknown.
  bool cancel(const OrderId id);

  // Reduce an order's quantity by `by`. It keeps its place in the queue.
  // Returns false if `id` is unknown or by <= 0. If its quantity reaches 0, it
  // is removed.
  bool reduce(const OrderId id, const Qty by);

  std::optional<Level> best_bid() const;
  std::optional<Level> best_ask() const;
  std::optional<OrderId> front(
      const Side side) const;  // next order to be filled
  Qty volume_at(const Side side, const Price price) const;
  std::vector<Level> depth(const Side side,
                           const std::size_t n) const;  // best first
  std::size_t order_count() const;

  friend std::ostream& operator<<(std::ostream& out, const OrderBook& ob);

 private:
  // Stores the levels
  std::map<Price, Level, std::greater<Price>> bids_;
  std::map<Price, Level> asks_;

  std::unordered_map<OrderId, OrderLocation> orders_;
};

}  // namespace orderbook
