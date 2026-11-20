#include <iostream>
#include <memory>

#include "src/order_book.hpp"

std::ostream& out = std::cout;

int main() {
  std::unique_ptr<orderbook::OrderBook> order_book =
      std::make_unique<orderbook::OrderBook>();

  order_book->Add(1, orderbook::Side::kBuy, 25, 5);
  order_book->Add(2, orderbook::Side::kBuy, 29, 5);
  order_book->Add(3, orderbook::Side::kBuy, 28, 5);
  order_book->Add(4, orderbook::Side::kSell, 20, 5);
  order_book->Add(5, orderbook::Side::kBuy, 21, 5);
  order_book->Add(6, orderbook::Side::kBuy, 220, 5);
  order_book->Add(7, orderbook::Side::kBuy, 21, 5);
  order_book->Add(8, orderbook::Side::kSell, 10, 5);

  out << *order_book;

  order_book->Cancel(1);
  order_book->Cancel(2);

  order_book->Front(orderbook::Side::kBuy);

  order_book->BestAsk();
  order_book->BestBid();

  out << *order_book;
}
