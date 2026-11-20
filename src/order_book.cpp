#include "src/order_book.hpp"

#include <iostream>
#include <vector>

namespace orderbook {

bool OrderBook::Add(const OrderId id, const Side side, const Price price,
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

  if (side == Side::kBuy) {
    add_to_side(bids_);
  } else {
    add_to_side(asks_);
  }
}

bool OrderBook::Cancel(const OrderId id) {
  if (!orders_.contains(id)) return false;

  OrderLocation order_loc = orders_[id];
  order_loc.orders->erase(order_loc.it);
  orders_.erase(id);

  return true;
}

bool OrderBook::Reduce(const OrderId id, const Qty by) {}

std::optional<Level> OrderBook::BestBid() const {
  if (bids_.size() == 0) {
    return std::nullopt;
  } else {
    return bids_.begin()->second;
  }
}

std::optional<Level> OrderBook::BestAsk() const {
  if (asks_.size() == 0) {
    return std::nullopt;
  } else {
    return asks_.begin()->second;
  }
}

std::optional<OrderId> OrderBook::Front(const Side side) const {
  if (side == Side::kBuy) {
    if (auto lowest_ask = BestAsk()) {
      auto front_order = lowest_ask->orders.front();
      return front_order.id;
    }
  }
  if (side == Side::kSell) {
    if (auto highest_bid = BestBid()) {
      auto front_order = highest_bid->orders.front();
      return front_order.id;
    }
  }
}

Qty OrderBook::VolumeAt(const Side side, const Price price) const {}
std::vector<Level> OrderBook::Depth(const Side side,
                                    const std::size_t n) const {}
std::size_t OrderBook::OrderCount() const {}

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
