#include "order_book.hpp"

bool OrderBook::add(OrderId id, Side side, Price price, Qty qty) {

};

bool OrderBook::cancel(OrderId id) {

};

bool OrderBook::reduce(OrderId id, Qty by) {};

std::optional<Level> OrderBook::best_bid() const {};
std::optional<Level> OrderBook::best_ask() const {};
std::optional<OrderId> OrderBook::front(Side side) const {};
Qty OrderBook::volume_at(Side side, Price price) const {};
std::vector<Level> OrderBook::depth(Side side, std::size_t n) const {};
std::size_t OrderBook::order_count() const {};
