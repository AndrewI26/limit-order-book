#include <iostream>
#include <memory>

#include "src/order_book.hpp"

std::ostream& out = std::cout;

int main() {
  std::unique_ptr<orderbook::OrderBook> orderBook =
      std::make_unique<orderbook::OrderBook>();

  orderBook->add(1, orderbook::Side::Buy, 25, 5);
  orderBook->add(2, orderbook::Side::Buy, 29, 5);
  orderBook->add(3, orderbook::Side::Buy, 28, 5);
  orderBook->add(4, orderbook::Side::Sell, 20, 5);
  orderBook->add(5, orderbook::Side::Buy, 21, 5);
  orderBook->add(6, orderbook::Side::Buy, 220, 5);
  orderBook->add(7, orderbook::Side::Buy, 21, 5);
  orderBook->add(8, orderbook::Side::Sell, 10, 5);

  out << *orderBook;

  orderBook->cancel(1);
  orderBook->cancel(2);

  orderBook->front(orderbook::Side::Buy);

  orderBook->best_ask();
  orderBook->best_bid();

  out << *orderBook;
}
