#include <cstdint>
#include <optional>
#include <vector>

using OrderId = std::uint64_t;
using Price = std::uint64_t;
using Qty = std::uint64_t;

enum class Side : std::uint8_t { Buy, Sell };

struct Level {
  Price price;
  Qty qty;             // Total resting quantity at this price.
  std::uint32_t count; // Number of orders at this price.
  bool operator==(const Level&) const = default;
};

class OrderBook {
public:
  // Add a resting limit order at the back of its price level's queue.
  // Returns false (and changes nothing) if `id` already exists or qty <= 0.
  bool add(OrderId id, Side side, Price price, Qty qty);

  // Remove an order entirely. Returns false if `id` is unknown.
  bool cancel(OrderId id);

  // Reduce an order's quantity by `by`. It keeps its place in the queue.
  // Returns false if `id` is unknown or by <= 0. If its quantity reaches 0, it
  // is removed.
  bool reduce(OrderId id, Qty by);

  std::optional<Level> best_bid() const;
  std::optional<Level> best_ask() const;
  std::optional<OrderId> front(Side side) const; // next order to be filled
  Qty volume_at(Side side, Price price) const;
  std::vector<Level> depth(Side side, std::size_t n) const; // best first
  std::size_t order_count() const;
};
