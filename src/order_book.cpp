#include "src/order_book.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace orderbook {

bool OrderBook::Add(const OrderId id, const Side side, const Price price,
                    const Qty qty) {
  if (qty == 0 || orders_.contains(id)) {
    return false;
  }

  auto add_to_side = [&](auto& levels) -> bool {
    Level& level = levels[price];
    level.orders.emplace_back(id, side, price, qty);

    auto order_it = std::prev(level.orders.end());

    orders_[id] = {order_it, &level.orders};
    level.qty += qty;
    level.count++;
    return true;
  };

  if (side == Side::Buy) {
    return add_to_side(bids_);
  }
  return add_to_side(asks_);
}

void OrderBook::RemoveFromLevel(LevelQueue::iterator it) {
  // Copy what we need before `it` is invalidated by the erase.
  const Price price = it->price;
  const Qty qty = it->qty;

  auto remove_from_side = [&](auto& levels) {
    auto level_it = levels.find(price);
    Level& level = level_it->second;
    level.qty -= qty;
    level.count--;
    level.orders.erase(it);
    if (level.orders.empty()) levels.erase(level_it);
  };

  if (it->side == Side::Buy) {
    remove_from_side(bids_);
  } else {
    remove_from_side(asks_);
  }
}

bool OrderBook::Cancel(const OrderId id) {
  auto found = orders_.find(id);
  if (found == orders_.end()) return false;

  RemoveFromLevel(found->second.it);
  orders_.erase(found);
  return true;
}

bool OrderBook::Reduce(const OrderId id, const Qty by) {
  if (by == 0) return false;
  auto found = orders_.find(id);
  if (found == orders_.end()) return false;

  Order& order = *found->second.it;
  if (by >= order.qty) {
    RemoveFromLevel(found->second.it);
    orders_.erase(found);
    return true;
  }

  // Reducing keeps the order's place in the queue, so only the totals change.
  Level& level =
      order.side == Side::Buy ? bids_.at(order.price) : asks_.at(order.price);
  order.qty -= by;
  level.qty -= by;
  return true;
}

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
  if (side == Side::Buy) {
    if (auto lowest_ask = BestAsk()) {
      auto front_order = lowest_ask->orders.front();
      return front_order.id;
    }
  }
  if (side == Side::Sell) {
    if (auto highest_bid = BestBid()) {
      auto front_order = highest_bid->orders.front();
      return front_order.id;
    }
  }
  return std::nullopt;
}

Qty OrderBook::VolumeAt(const Side side, const Price price) const {
  auto volume_on_side = [&](const auto& levels) -> Qty {
    auto level_it = levels.find(price);
    return level_it == levels.end() ? 0 : level_it->second.qty;
  };

  if (side == Side::Buy) return volume_on_side(bids_);
  return volume_on_side(asks_);
}

std::vector<Level> OrderBook::Depth(const Side side,
                                    const std::size_t n) const {
  // Both maps are ordered best price first, so take the first n levels.
  auto depth_on_side = [&](const auto& levels) {
    std::vector<Level> result;
    result.reserve(std::min(n, levels.size()));
    for (const auto& [price, level] : levels) {
      if (result.size() == n) break;
      result.push_back(level);
    }
    return result;
  };

  if (side == Side::Buy) return depth_on_side(bids_);
  return depth_on_side(asks_);
}

std::size_t OrderBook::OrderCount() const { return orders_.size(); }

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
