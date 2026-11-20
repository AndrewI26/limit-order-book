#include <iostream>
#include <memory>

#include "src/order_book.hpp"

std::ostream& out = std::cout;

int main() {
  std::unique_ptr<orderbook::OrderBook> order_book =
      std::make_unique<orderbook::OrderBook>();

  order_book->Add(1, orderbook::Side::Buy, 25, 5);
  order_book->Add(2, orderbook::Side::Buy, 29, 5);
  order_book->Add(3, orderbook::Side::Buy, 28, 5);
  order_book->Add(4, orderbook::Side::Sell, 20, 5);
  order_book->Add(5, orderbook::Side::Buy, 21, 5);
  order_book->Add(6, orderbook::Side::Buy, 220, 5);
  order_book->Add(7, orderbook::Side::Buy, 21, 5);
  order_book->Add(8, orderbook::Side::Sell, 10, 5);

  out << *order_book;

  order_book->Cancel(1);
  order_book->Cancel(2);

  order_book->Front(orderbook::Side::Buy);

  order_book->BestAsk();
  order_book->BestBid();

  out << *order_book;
}
