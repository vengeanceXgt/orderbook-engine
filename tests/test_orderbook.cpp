#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include "orderbook.hpp"

using namespace ob;

static int checks = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        ++checks;                                                            \
        if (!(cond)) {                                                       \
            std::cerr << "FAILED: " #cond " (line " << __LINE__ << ")\n";    \
            std::exit(1);                                                    \
        }                                                                    \
    } while (0)

void test_rest_and_best_prices() {
    OrderBook b;
    b.add_limit(1, Side::Buy, 100, 10);
    b.add_limit(2, Side::Buy, 101, 5);
    b.add_limit(3, Side::Sell, 105, 7);
    CHECK(*b.best_bid() == 101);
    CHECK(*b.best_ask() == 105);
    CHECK(b.resting_orders() == 3);
}

void test_price_priority() {
    std::vector<Trade> trades;
    OrderBook b([&](const Trade& t) { trades.push_back(t); });
    b.add_limit(1, Side::Sell, 102, 5);
    b.add_limit(2, Side::Sell, 101, 5);  // better price, arrives later
    b.add_limit(3, Side::Buy, 102, 5);
    CHECK(trades.size() == 1);
    CHECK(trades[0].maker_id == 2 && trades[0].price == 101);
}

void test_time_priority_within_level() {
    std::vector<Trade> trades;
    OrderBook b([&](const Trade& t) { trades.push_back(t); });
    b.add_limit(1, Side::Sell, 100, 5);
    b.add_limit(2, Side::Sell, 100, 5);
    b.add_limit(3, Side::Buy, 100, 5);
    CHECK(trades.size() == 1 && trades[0].maker_id == 1);
    CHECK(b.depth_at(Side::Sell, 100) == 5);
}

void test_partial_fill_rests_remainder() {
    OrderBook b;
    b.add_limit(1, Side::Sell, 100, 4);
    Quantity filled = b.add_limit(2, Side::Buy, 100, 10);
    CHECK(filled == 4);
    CHECK(!b.best_ask().has_value());
    CHECK(*b.best_bid() == 100 && b.depth_at(Side::Buy, 100) == 6);
}

void test_trade_executes_at_maker_price() {
    std::vector<Trade> trades;
    OrderBook b([&](const Trade& t) { trades.push_back(t); });
    b.add_limit(1, Side::Sell, 100, 5);
    b.add_limit(2, Side::Buy, 110, 5);  // willing to pay more
    CHECK(trades.size() == 1 && trades[0].price == 100);
}

void test_market_order_sweeps_levels() {
    OrderBook b;
    b.add_limit(1, Side::Sell, 100, 3);
    b.add_limit(2, Side::Sell, 101, 3);
    b.add_limit(3, Side::Sell, 102, 3);
    Quantity filled = b.add_market(4, Side::Buy, 7);
    CHECK(filled == 7);
    CHECK(*b.best_ask() == 102 && b.depth_at(Side::Sell, 102) == 2);
}

void test_market_order_unfilled_remainder_discarded() {
    OrderBook b;
    b.add_limit(1, Side::Sell, 100, 3);
    CHECK(b.add_market(2, Side::Buy, 10) == 3);
    CHECK(b.resting_orders() == 0);
}

void test_cancel() {
    OrderBook b;
    b.add_limit(1, Side::Buy, 100, 5);
    b.add_limit(2, Side::Buy, 100, 5);
    CHECK(b.cancel(1));
    CHECK(!b.cancel(1));        // already gone
    CHECK(!b.cancel(999));      // unknown
    CHECK(b.depth_at(Side::Buy, 100) == 5);
    CHECK(b.cancel(2));
    CHECK(!b.best_bid().has_value());
}

void test_duplicate_id_rejected() {
    OrderBook b;
    b.add_limit(1, Side::Buy, 100, 5);
    CHECK(b.add_limit(1, Side::Buy, 100, 5) == 0);
    CHECK(b.depth_at(Side::Buy, 100) == 5);
}

// Randomised invariant test: the book is never crossed, and quantity is never created from nothing.
void test_random_flow_invariants() {
    std::mt19937_64 rng(42);
    Quantity total_submitted = 0, total_traded = 0;
    OrderBook b([&](const Trade& t) { total_traded += t.qty; });
    std::vector<OrderId> live;
    for (OrderId id = 1; id <= 50'000; ++id) {
        const int action = rng() % 10;
        if (action < 8) {
            Side s = (rng() & 1) ? Side::Buy : Side::Sell;
            Price p = 1000 + static_cast<Price>(rng() % 21) - 10;
            Quantity q = 1 + rng() % 20;
            total_submitted += q;
            b.add_limit(id, s, p, q);
            live.push_back(id);
        } else if (!live.empty()) {
            size_t i = rng() % live.size();
            b.cancel(live[i]);
            live[i] = live.back();
            live.pop_back();
        }
        if (b.best_bid() && b.best_ask()) CHECK(*b.best_bid() < *b.best_ask());
    }
    // Each trade consumes its quantity from both the taker and the maker, so
    // resting + 2 * traded can never exceed what was submitted (cancels only reduce resting).
    Quantity resting = 0;
    for (Side s : {Side::Buy, Side::Sell})
        for (Price p = 990; p <= 1010; ++p) resting += b.depth_at(s, p);
    CHECK(resting + 2 * total_traded <= total_submitted);
}

int main() {
    test_rest_and_best_prices();
    test_price_priority();
    test_time_priority_within_level();
    test_partial_fill_rests_remainder();
    test_trade_executes_at_maker_price();
    test_market_order_sweeps_levels();
    test_market_order_unfilled_remainder_discarded();
    test_cancel();
    test_duplicate_id_rejected();
    test_random_flow_invariants();
    std::cout << "All " << checks << " checks passed\n";
}
