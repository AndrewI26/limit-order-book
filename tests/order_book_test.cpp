#include "src/order_book.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace orderbook {
namespace {

// ---- Add --------------------------------------------------------------------

TEST(AddTest, AcceptsNewOrder) {
  OrderBook book;
  EXPECT_TRUE(book.Add(1, Side::Buy, 100, 10));
  EXPECT_EQ(book.OrderCount(), 1u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 10u);
}

TEST(AddTest, RejectsZeroQuantity) {
  OrderBook book;
  EXPECT_FALSE(book.Add(1, Side::Buy, 100, 0));
  EXPECT_EQ(book.OrderCount(), 0u);
  EXPECT_EQ(book.BestBid(), nullptr);
}

TEST(AddTest, RejectsDuplicateIdAndChangesNothing) {
  OrderBook book;
  ASSERT_TRUE(book.Add(1, Side::Buy, 100, 10));
  EXPECT_FALSE(book.Add(1, Side::Sell, 105, 3));
  EXPECT_EQ(book.OrderCount(), 1u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 10u);
  EXPECT_EQ(book.BestAsk(), nullptr);
}

TEST(AddTest, AggregatesOrdersAtSamePrice) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 4);
  book.Add(2, Side::Sell, 101, 6);
  auto ask = book.BestAsk();
  ASSERT_NE(ask, nullptr);
  EXPECT_EQ(ask->qty, 10u);
  EXPECT_EQ(ask->count, 2u);
}

TEST(AddTest, QueuesOrdersAtSamePriceInArrivalOrder) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 1);
  book.Add(2, Side::Buy, 100, 1);
  book.Add(3, Side::Buy, 100, 1);
  auto bid = book.BestBid();
  ASSERT_NE(bid, nullptr);
  std::vector<OrderId> ids;
  for (const Order& order : bid->orders) ids.push_back(order.id);
  EXPECT_EQ(ids, (std::vector<OrderId>{1, 2, 3}));
}

// ---- Matching ---------------------------------------------------------------

TEST(MatchTest, NonCrossingOrdersRestWithoutTrades) {
  OrderBook book;
  std::vector<Trade> trades;
  book.Add(1, Side::Sell, 101, 5, &trades);
  book.Add(2, Side::Buy, 100, 5, &trades);
  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.OrderCount(), 2u);
}

TEST(MatchTest, BuyFullyFillsAgainstRestingAsk) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 5);
  std::vector<Trade> trades;
  EXPECT_TRUE(book.Add(2, Side::Buy, 101, 5, &trades));
  EXPECT_EQ(trades, (std::vector<Trade>{{2, 1, 101, 5}}));
  EXPECT_EQ(book.OrderCount(), 0u);
  EXPECT_EQ(book.BestAsk(), nullptr);
  EXPECT_EQ(book.BestBid(), nullptr);
}

TEST(MatchTest, SellFullyFillsAgainstRestingBid) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 5);
  std::vector<Trade> trades;
  EXPECT_TRUE(book.Add(2, Side::Sell, 100, 5, &trades));
  EXPECT_EQ(trades, (std::vector<Trade>{{2, 1, 100, 5}}));
  EXPECT_EQ(book.OrderCount(), 0u);
}

TEST(MatchTest, FillsAtRestingPriceNotIncomingPrice) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 5);
  std::vector<Trade> trades;
  book.Add(2, Side::Buy, 110, 5, &trades);
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 101u);
}

TEST(MatchTest, PartialFillRestsRemainder) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 3);
  std::vector<Trade> trades;
  book.Add(2, Side::Buy, 101, 10, &trades);
  EXPECT_EQ(trades, (std::vector<Trade>{{2, 1, 101, 3}}));
  EXPECT_EQ(book.BestAsk(), nullptr);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 101), 7u);
  EXPECT_EQ(book.Front(Side::Sell), 2u);
}

TEST(MatchTest, PartiallyFilledMakerKeepsQueuePosition) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 10);
  book.Add(2, Side::Sell, 101, 10);
  book.Add(3, Side::Buy, 101, 4);
  const Level* ask = book.BestAsk();
  ASSERT_NE(ask, nullptr);
  EXPECT_EQ(ask->orders.front().id, 1u);
  EXPECT_EQ(ask->orders.front().qty, 6u);
  EXPECT_EQ(ask->qty, 16u);
  EXPECT_EQ(ask->count, 2u);
}

TEST(MatchTest, FillsInTimePriorityWithinLevel) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 2);
  book.Add(2, Side::Buy, 100, 2);
  book.Add(3, Side::Buy, 100, 2);
  std::vector<Trade> trades;
  book.Add(4, Side::Sell, 100, 5, &trades);
  EXPECT_EQ(trades, (std::vector<Trade>{
                        {4, 1, 100, 2}, {4, 2, 100, 2}, {4, 3, 100, 1}}));
  EXPECT_EQ(book.OrderCount(), 1u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 1u);
}

TEST(MatchTest, SweepsMultipleLevelsBestPriceFirst) {
  OrderBook book;
  book.Add(1, Side::Sell, 103, 5);
  book.Add(2, Side::Sell, 101, 5);
  book.Add(3, Side::Sell, 102, 5);
  std::vector<Trade> trades;
  book.Add(4, Side::Buy, 102, 12, &trades);
  EXPECT_EQ(trades, (std::vector<Trade>{{4, 2, 101, 5}, {4, 3, 102, 5}}));
  // 103 doesn't cross, so the remaining 2 rest as a bid at 102.
  EXPECT_EQ(book.VolumeAt(Side::Sell, 103), 5u);
  EXPECT_EQ(book.VolumeAt(Side::Sell, 101), 0u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 102), 2u);
  EXPECT_EQ(book.Depth(Side::Sell, 10).size(), 1u);
}

TEST(MatchTest, FilledMakersAreRemovedFromLookup) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 5);
  book.Add(2, Side::Buy, 101, 5);
  EXPECT_FALSE(book.Cancel(1));
  EXPECT_FALSE(book.Reduce(1, 1));
  EXPECT_TRUE(book.Add(1, Side::Buy, 99, 1));  // Id is free again.
}

TEST(MatchTest, WorksWithoutTradesOutput) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 5);
  EXPECT_TRUE(book.Add(2, Side::Buy, 101, 2));
  EXPECT_EQ(book.VolumeAt(Side::Sell, 101), 3u);
}

TEST(MatchTest, RejectedOrderDoesNotTrade) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 5);
  book.Add(2, Side::Buy, 99, 5);
  std::vector<Trade> trades;
  EXPECT_FALSE(book.Add(2, Side::Buy, 101, 5, &trades));  // Duplicate id.
  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.VolumeAt(Side::Sell, 101), 5u);
}

// ---- Cancel -----------------------------------------------------------------

TEST(CancelTest, RemovesOrderAndUpdatesLevel) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  book.Add(2, Side::Buy, 100, 5);
  EXPECT_TRUE(book.Cancel(1));
  EXPECT_EQ(book.OrderCount(), 1u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 5u);
  EXPECT_EQ(book.BestBid()->count, 1u);
}

TEST(CancelTest, ErasesLevelWhenLastOrderCancelled) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 3);
  book.Add(2, Side::Sell, 102, 4);
  EXPECT_TRUE(book.Cancel(1));
  EXPECT_EQ(book.VolumeAt(Side::Sell, 101), 0u);
  EXPECT_EQ(book.Depth(Side::Sell, 10).size(), 1u);
  EXPECT_EQ(book.BestAsk()->qty, 4u);
}

TEST(CancelTest, ReturnsFalseForUnknownId) {
  OrderBook book;
  EXPECT_FALSE(book.Cancel(42));
}

TEST(CancelTest, ReturnsFalseWhenCancelledTwice) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  EXPECT_TRUE(book.Cancel(1));
  EXPECT_FALSE(book.Cancel(1));
}

TEST(CancelTest, IdCanBeReusedAfterCancel) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  book.Cancel(1);
  EXPECT_TRUE(book.Add(1, Side::Sell, 105, 2));
}

// ---- Reduce -----------------------------------------------------------------

TEST(ReduceTest, PartialReduceUpdatesQuantities) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  book.Add(2, Side::Buy, 100, 5);
  EXPECT_TRUE(book.Reduce(1, 4));
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 11u);
  EXPECT_EQ(book.BestBid()->count, 2u);
  EXPECT_EQ(book.BestBid()->orders.front().qty, 6u);
}

TEST(ReduceTest, PartialReduceKeepsQueuePosition) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  book.Add(2, Side::Buy, 100, 5);
  book.Reduce(1, 9);
  EXPECT_EQ(book.BestBid()->orders.front().id, 1u);
}

TEST(ReduceTest, ReduceToZeroRemovesOrder) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 3);
  EXPECT_TRUE(book.Reduce(1, 3));
  EXPECT_EQ(book.OrderCount(), 0u);
  EXPECT_EQ(book.BestAsk(), nullptr);
}

TEST(ReduceTest, ReduceByMoreThanQuantityRemovesOrder) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 3);
  book.Add(2, Side::Sell, 101, 2);
  EXPECT_TRUE(book.Reduce(1, 100));
  EXPECT_EQ(book.OrderCount(), 1u);
  EXPECT_EQ(book.VolumeAt(Side::Sell, 101), 2u);
}

TEST(ReduceTest, ReturnsFalseForZeroAmount) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  EXPECT_FALSE(book.Reduce(1, 0));
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 10u);
}

TEST(ReduceTest, ReturnsFalseForUnknownId) {
  OrderBook book;
  EXPECT_FALSE(book.Reduce(42, 1));
}

// ---- BestBid / BestAsk ------------------------------------------------------

TEST(BestPriceTest, EmptyBookHasNoBestBidOrAsk) {
  OrderBook book;
  EXPECT_EQ(book.BestBid(), nullptr);
  EXPECT_EQ(book.BestAsk(), nullptr);
}

TEST(BestPriceTest, BestBidIsHighestBuyPrice) {
  OrderBook book;
  book.Add(1, Side::Buy, 99, 1);
  book.Add(2, Side::Buy, 101, 2);
  book.Add(3, Side::Buy, 100, 3);
  auto bid = book.BestBid();
  ASSERT_NE(bid, nullptr);
  EXPECT_EQ(bid->orders.front().price, 101u);
  EXPECT_EQ(bid->qty, 2u);
}

TEST(BestPriceTest, BestAskIsLowestSellPrice) {
  OrderBook book;
  book.Add(1, Side::Sell, 105, 1);
  book.Add(2, Side::Sell, 103, 2);
  book.Add(3, Side::Sell, 104, 3);
  auto ask = book.BestAsk();
  ASSERT_NE(ask, nullptr);
  EXPECT_EQ(ask->orders.front().price, 103u);
  EXPECT_EQ(ask->qty, 2u);
}

TEST(BestPriceTest, BuysDoNotAffectBestAskAndViceVersa) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 1);
  EXPECT_EQ(book.BestAsk(), nullptr);
  book.Add(2, Side::Sell, 101, 1);
  EXPECT_EQ(book.BestBid()->orders.front().id, 1u);
  EXPECT_EQ(book.BestAsk()->orders.front().id, 2u);
}

TEST(BestPriceTest, ReturnsPointerIntoBookNotCopy) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 5);
  const Level* bid = book.BestBid();
  ASSERT_NE(bid, nullptr);
  EXPECT_EQ(bid, book.Depth(Side::Buy, 1).front());
  book.Add(2, Side::Buy, 100, 3);  // Same level: node is stable.
  EXPECT_EQ(bid->qty, 8u);
}

// ---- Front ------------------------------------------------------------------
// Front(side) currently returns the first order on the *opposite* side of the
// book: Front(Buy) looks at the best ask, Front(Sell) at the best bid.

TEST(FrontTest, EmptyBookReturnsNullopt) {
  OrderBook book;
  EXPECT_EQ(book.Front(Side::Buy), std::nullopt);
  EXPECT_EQ(book.Front(Side::Sell), std::nullopt);
}

TEST(FrontTest, BuyReturnsFirstOrderAtBestAsk) {
  OrderBook book;
  book.Add(1, Side::Sell, 102, 1);
  book.Add(2, Side::Sell, 101, 1);
  book.Add(3, Side::Sell, 101, 1);
  EXPECT_EQ(book.Front(Side::Buy), 2u);
}

TEST(FrontTest, SellReturnsFirstOrderAtBestBid) {
  OrderBook book;
  book.Add(1, Side::Buy, 99, 1);
  book.Add(2, Side::Buy, 100, 1);
  book.Add(3, Side::Buy, 100, 1);
  EXPECT_EQ(book.Front(Side::Sell), 2u);
}

TEST(FrontTest, AdvancesWhenFrontOrderCancelled) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 1);
  book.Add(2, Side::Buy, 100, 1);
  book.Cancel(1);
  EXPECT_EQ(book.Front(Side::Sell), 2u);
}

TEST(FrontTest, ReturnsNulloptAfterLastOrderCancelled) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 1);
  book.Cancel(1);
  EXPECT_EQ(book.Front(Side::Sell), std::nullopt);
}

// ---- VolumeAt ---------------------------------------------------------------

TEST(VolumeAtTest, SumsAllOrdersAtPrice) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  book.Add(2, Side::Buy, 100, 5);
  book.Add(3, Side::Buy, 99, 7);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 100), 15u);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 99), 7u);
}

TEST(VolumeAtTest, ReturnsZeroForEmptyPrice) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  EXPECT_EQ(book.VolumeAt(Side::Buy, 101), 0u);
}

TEST(VolumeAtTest, SidesAreIndependent) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 10);
  EXPECT_EQ(book.VolumeAt(Side::Sell, 100), 0u);
}

// ---- Depth ------------------------------------------------------------------

TEST(DepthTest, EmptyBookHasNoLevels) {
  OrderBook book;
  EXPECT_TRUE(book.Depth(Side::Buy, 5).empty());
  EXPECT_TRUE(book.Depth(Side::Sell, 5).empty());
}

TEST(DepthTest, BidsAreBestFirst) {
  OrderBook book;
  book.Add(1, Side::Buy, 98, 1);
  book.Add(2, Side::Buy, 100, 2);
  book.Add(3, Side::Buy, 99, 3);
  auto levels = book.Depth(Side::Buy, 10);
  ASSERT_EQ(levels.size(), 3u);
  EXPECT_EQ(levels[0]->orders.front().price, 100u);
  EXPECT_EQ(levels[1]->orders.front().price, 99u);
  EXPECT_EQ(levels[2]->orders.front().price, 98u);
}

TEST(DepthTest, AsksAreBestFirst) {
  OrderBook book;
  book.Add(1, Side::Sell, 103, 1);
  book.Add(2, Side::Sell, 101, 2);
  book.Add(3, Side::Sell, 102, 3);
  auto levels = book.Depth(Side::Sell, 10);
  ASSERT_EQ(levels.size(), 3u);
  EXPECT_EQ(levels[0]->orders.front().price, 101u);
  EXPECT_EQ(levels[1]->orders.front().price, 102u);
  EXPECT_EQ(levels[2]->orders.front().price, 103u);
}

TEST(DepthTest, LimitsToNLevels) {
  OrderBook book;
  book.Add(1, Side::Sell, 101, 1);
  book.Add(2, Side::Sell, 102, 1);
  book.Add(3, Side::Sell, 103, 1);
  auto levels = book.Depth(Side::Sell, 2);
  ASSERT_EQ(levels.size(), 2u);
  EXPECT_EQ(levels[1]->orders.front().price, 102u);
}

TEST(DepthTest, ZeroLevelsReturnsEmpty) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 1);
  EXPECT_TRUE(book.Depth(Side::Buy, 0).empty());
}

TEST(DepthTest, LevelsCarryAggregateQuantityAndCount) {
  OrderBook book;
  book.Add(1, Side::Buy, 100, 4);
  book.Add(2, Side::Buy, 100, 6);
  auto levels = book.Depth(Side::Buy, 1);
  ASSERT_EQ(levels.size(), 1u);
  EXPECT_EQ(levels[0]->qty, 10u);
  EXPECT_EQ(levels[0]->count, 2u);
}

// ---- OrderCount -------------------------------------------------------------

TEST(OrderCountTest, TracksAddsCancelsAndFullReduces) {
  OrderBook book;
  EXPECT_EQ(book.OrderCount(), 0u);
  book.Add(1, Side::Buy, 100, 5);
  book.Add(2, Side::Sell, 101, 5);
  book.Add(3, Side::Sell, 102, 5);
  EXPECT_EQ(book.OrderCount(), 3u);
  book.Cancel(1);
  EXPECT_EQ(book.OrderCount(), 2u);
  book.Reduce(2, 1);  // Partial: order stays.
  EXPECT_EQ(book.OrderCount(), 2u);
  book.Reduce(2, 4);  // Full: order removed.
  EXPECT_EQ(book.OrderCount(), 1u);
}

// ---- operator<< -------------------------------------------------------------

TEST(PrintTest, PrintsAsksHighToLowThenBidsHighToLow) {
  OrderBook book;
  book.Add(1, Side::Buy, 99, 5);
  book.Add(2, Side::Buy, 100, 3);
  book.Add(3, Side::Buy, 100, 2);
  book.Add(4, Side::Sell, 102, 7);
  book.Add(5, Side::Sell, 101, 1);

  std::ostringstream out;
  out << book;
  EXPECT_EQ(out.str(),
            "=== ORDER BOOK ===\n"
            "ASKS:\n"
            "  102 | qty: 7 (1 orders)\n"
            "  101 | qty: 1 (1 orders)\n"
            "------------------\n"
            "BIDS:\n"
            "  100 | qty: 5 (2 orders)\n"
            "  99 | qty: 5 (1 orders)\n");
}

}  // namespace
}  // namespace orderbook
