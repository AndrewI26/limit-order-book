#include "src/order_book.hpp"

#include <iostream>
#include <vector>

namespace orderbook {

bool OrderBook::add(const OrderId id, const Side side, const Price price,
                    const Qty qty) {
  if (orders_.contains(id)) {
    return false;
  }

  auto add_to_side = [&](auto& levels) -> bool {
    Level& level = levels[price];
    orderbook::Order& order = level.orders.emplace_back(id, side, price, qty);

    auto order_it = std::prev(level.orders.end());

    orders_[id] = {order_it, &level.orders};
    level.qty += qty;
    level.count++;
    return true;
  };

  if (side == Side::Buy) {
    add_to_side(bids_);
  } else {
    add_to_side(asks_);
  }
}

bool OrderBook::cancel(const OrderId id) {
  if (!orders_.contains(id)) return false;

  OrderLocation order_loc = orders_[id];
  order_loc.orders->erase(order_loc.it);
  orders_.erase(id);

  return true;
}

bool OrderBook::reduce(const OrderId id, const Qty by) {}

std::optional<Level> OrderBook::best_bid() const {}
std::optional<Level> OrderBook::best_ask() const {}
std::optional<OrderId> OrderBook::front(const Side side) const {}
Qty OrderBook::volume_at(const Side side, const Price price) const {}
std::vector<Level> OrderBook::depth(const Side side,
                                    const std::size_t n) const {}
std::size_t OrderBook::order_count() const {}

std::ostream& operator<<(std::ostream& out, const OrderBook& ob) {
  out << "=== ORDER BOOK ===\n";

  // Print asks from highest price down to best (lowest) ask
  out << "ASKS:\n";
  for (auto it = ob.asks_.rbegin(); it != ob.asks_.rend(); ++it) {
    const auto& [price, level] = *it;
    out << "  " << price << " | qty: " << level.qty << " (" << level.count
        << " orders)\n";
  }

  out << "------------------\n";

  // Print bids from best (highest) bid down to lowest price
  out << "BIDS:\n";
  for (const auto& [price, level] : ob.bids_) {
    out << "  " << price << " | qty: " << level.qty << " (" << level.count
        << " orders)\n";
  }

  return out;
}

}  // namespace orderbook
