#include "src/order_book.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace orderbook {

template <typename Levels, typename Crosses>
Qty OrderBook::Match(const OrderId id, Qty qty, Levels& levels, Crosses crosses,
                     std::vector<Trade>* trades) {
  // Levels are ordered best price first, so walk from the front.
  while (qty > 0 && !levels.empty()) {
    auto level_it = levels.begin();
    if (!crosses(level_it->first)) break;

    Level& level = level_it->second;
    while (qty > 0 && !level.orders.empty()) {
      Order& maker = level.orders.front();
      const Qty fill = std::min(qty, maker.qty);
      if (trades != nullptr) {
        trades->push_back({id, maker.id, maker.price, fill});
      }

      qty -= fill;
      maker.qty -= fill;
      level.qty -= fill;
      if (maker.qty == 0) {
        orders_.erase(maker.id);
        level.orders.pop_front();
        level.count--;
      }
    }
    if (level.orders.empty()) levels.erase(level_it);
  }
  return qty;
}

bool OrderBook::Add(const OrderId id, const Side side, const Price price,
                    const Qty qty, std::vector<Trade>* trades) {
  if (qty == 0 || orders_.contains(id)) {
    return false;
  }

  const Qty remaining =
      side == Side::Buy
          ? Match(
                id, qty, asks_, [price](Price ask) { return ask <= price; },
                trades)
          : Match(
                id, qty, bids_, [price](Price bid) { return bid >= price; },
                trades);
  if (remaining == 0) return true;

  auto rest_on_side = [&](auto& levels) {
    Level& level = levels[price];
    level.orders.emplace_back(id, side, price, remaining);
    orders_[id] = {std::prev(level.orders.end()), &level.orders};
    level.qty += remaining;
    level.count++;
  };

  if (side == Side::Buy) {
    rest_on_side(bids_);
  } else {
    rest_on_side(asks_);
  }
  return true;
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

const Level* OrderBook::BestBid() const {
  return bids_.empty() ? nullptr : &bids_.begin()->second;
}

const Level* OrderBook::BestAsk() const {
  return asks_.empty() ? nullptr : &asks_.begin()->second;
}

std::optional<OrderId> OrderBook::Front(const Side side) const {
  const Level* level = side == Side::Buy ? BestAsk() : BestBid();
  if (level == nullptr) return std::nullopt;
  return level->orders.front().id;
}

Qty OrderBook::VolumeAt(const Side side, const Price price) const {
  auto volume_on_side = [&](const auto& levels) -> Qty {
    auto level_it = levels.find(price);
    return level_it == levels.end() ? 0 : level_it->second.qty;
  };

  if (side == Side::Buy) return volume_on_side(bids_);
  return volume_on_side(asks_);
}

std::vector<const Level*> OrderBook::Depth(const Side side,
                                           const std::size_t n) const {
  // Both maps are ordered best price first, so take the first n levels.
  auto depth_on_side = [&](const auto& levels) {
    std::vector<const Level*> result;
    result.reserve(std::min(n, levels.size()));
    for (const auto& [price, level] : levels) {
      if (result.size() == n) break;
      result.push_back(&level);
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
